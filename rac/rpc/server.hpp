#ifndef RAC_RPC_SERVER_HPP
#define RAC_RPC_SERVER_HPP

#include "rac/async/async_main.hpp"
#include "rac/async/check_error.hpp"
#include "rac/async/scheduled_task.hpp"
#include "rac/async/task.hpp"
#include "rac/net/inet_addr.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include "rac/rpc/dispatcher.hpp"
#include <cstdint>
#include <list>
#include <utility>
namespace rac
{
// RpcServer server("127.0.0.1", 8080);
// server.bind("Echo", [](const User& u) { return u; });
// server.start();

class RpcServer
{
  public:
	explicit RpcServer(const InetAddr& sock_addr)
		: stream_(checkError(Socket::socket(nullptr)), sock_addr)
	{
		int listen_fd = stream_.fd();
		Socket::setReuseAddr(listen_fd, true);
		checkError(Socket::bind(listen_fd, sock_addr, nullptr));
		checkError(Socket::listen(listen_fd, nullptr));
	}

	RpcServer(const std::string& ip, std::uint16_t port)
		: RpcServer(InetAddr{ip, port})
	{
	}

	RpcServer(RpcServer&&) = delete;

	template <typename Func> void bind(const std::string& method_name, Func&& f)
	{
		dispatcher_.bind(method_name, std::forward<Func>(f));
	}

	void start()
	{
		LOG_WARN << "RPC Server listening on "
				 << stream_.sock_addr().to_formatted_string();
		async_main(serverLoop());
	}

  private:
	Task<> handleClient(int conn_fd);

	Task<> serverLoop();

  private:
	Stream stream_;
	RpcDispatcher dispatcher_{};
};

inline Task<> RpcServer::handleClient(int conn_fd)
{
	Stream s{conn_fd};

	while (true)
	{
		while (s.read_buffer()->readableBytes() < sizeof(RpcHeaderWire))
		{
			auto rd_buf = co_await s.read();
			if (!rd_buf)
			{
				LOG_ERROR << "Connection closed by peer before reading "
							 "header in fd "
						  << stream_.fd();
				co_return;
			}
		}

		RpcHeader h = s.read_buffer()->peekRpcHeader();
		std::size_t total_len = h.header_len + h.body_len;

		while (s.read_buffer()->readableBytes() < total_len)
		{
			auto rd_buf = co_await s.read();
			if (!rd_buf)
			{
				LOG_ERROR << "Connection closed by peer before reading "
							 "body in fd "
						  << stream_.fd();
				co_return;
			}
		}

		s.read_buffer()->retrieve(h.header_len);
		std::size_t readable_before = s.read_buffer()->readableBytes();

		std::string method_name;
		DeserializeTraits<std::string>::deserialize(s.read_buffer(),
													&method_name);

		LOG_DEBUG << "Client requesting method: " << method_name;

		std::size_t consumed =
			readable_before - s.read_buffer()->readableBytes();
		std::uint32_t args_limit = h.body_len - consumed;

		std::size_t size_before = s.write_buffer()->readableBytes();

		dispatcher_.dispatch(method_name, s.read_buffer(), s.write_buffer(),
							 args_limit);

		h.body_len = static_cast<uint32_t>(s.write_buffer()->readableBytes() -
										   size_before);
		s.write_buffer()->prependRpcHeader(h);

		co_await s.write();
	}
}

inline Task<> RpcServer::serverLoop()
{
	std::list<ScheduledTask<Task<>>> connections;
	int listen_fd = stream_.fd();
	Event ev{.fd = listen_fd, .flags = EPOLLIN};
	auto ev_awaiter = EventLoop::loop().wait_event(ev);

	while (true)
	{
		co_await ev_awaiter;
		while (true)
		{
			int conn_fd = accept4(listen_fd, nullptr, nullptr,
								  SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (conn_fd == -1)
			{
				break;
			}
			int opt = 1;
			setsockopt(conn_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
			connections.emplace_back(handleClient(conn_fd));
		}

		if (connections.size() < 100) [[likely]]
		{
			continue;
		}
		for (auto iter = connections.begin(); iter != connections.end();)
		{
			if (iter->done())
			{
				iter->result(); //< consume result, such as throw exception
				iter = connections.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	co_return;
}
} // namespace rac

#endif