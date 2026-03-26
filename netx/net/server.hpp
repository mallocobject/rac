#ifndef NETX_NET_SERVER_HPP
#define NETX_NET_SERVER_HPP

#include "netx/async/async_main.hpp"
#include "netx/async/check_error.hpp"
#include "netx/async/task.hpp"
#include "netx/net/inet_addr.hpp"
#include "netx/net/scheduler.hpp"
#include "netx/net/socket.hpp"
#include "netx/net/stream.hpp"
#include <cstddef>
#include <cstdint>
#include <latch>
#include <thread>
#include <vector>
namespace netx
{
namespace net
{
template <typename Derived> class Server
{
  protected:
	Server() : stream_(async::checkError(net::Socket::socket(nullptr)))
	{
	}

  public:
	Server(Server&&) = delete;

	Derived& listen(const net::InetAddr& sock_addr)
	{
		stream_.bind(sock_addr);
		int listen_fd = stream_.fd();
		net::Socket::setReuseAddr(listen_fd, true);
		async::checkError(net::Socket::bind(listen_fd, sock_addr, nullptr));
		async::checkError(net::Socket::listen(listen_fd, nullptr));

		return *static_cast<Derived*>(this);
	}

	Derived& listen(const std::string& ip, std::uint16_t port)
	{
		return listen(net::InetAddr{ip, port});
	}

	Derived& loop(std::size_t loop_count = 1)
	{
		if (loop_count < 1)
		{
			LOG_FATAL << "loop count must more then 1";
			exit(EXIT_FAILURE);
		}
		loop_count_ = loop_count;
		return *static_cast<Derived*>(this);
	}

	void start()
	{
		std::latch start_latch{static_cast<std::ptrdiff_t>(loop_count_)};
		for (size_t idx = 0; idx < loop_count_; idx++)
		{
			loops_.emplace_back(
				[this, &start_latch]
				{
					net::Scheduler scheduler{};
					schedulers_ptr_.push_back(&scheduler);
					async_main(scheduler.schedulerLoop(start_latch));
				});
		}
		start_latch.wait();

		LOG_WARN << "Netx-Server listening on "
				 << stream_.sock_addr().to_formatted_string();

		async_main(serverLoop());
	}

  protected:
	async::Task<> handleClient(int conn_fd)
	{
		return static_cast<Derived*>(this)->handleClient(conn_fd);
	}

	async::Task<> serverLoop();

  protected:
	net::Stream stream_;
	std::size_t loop_count_{0};

	std::vector<net::Scheduler*> schedulers_ptr_;
	std::vector<std::jthread> loops_;
};

template <typename Derived> inline async::Task<> Server<Derived>::serverLoop()
{
	int listen_fd = stream_.fd();
	async::Event ev{.fd = listen_fd, .flags = EPOLLIN};
	auto ev_awaiter = async::EventLoop::loop().wait_event(ev);
	static std::size_t lucky_boy = 0;

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

			assert(schedulers_ptr_.size() >= 1);
			auto lucky = schedulers_ptr_[lucky_boy++ % schedulers_ptr_.size()];
			lucky->push(handleClient(conn_fd));
			lucky->wakeup();
		}
	}
	co_return;
}
} // namespace net
} // namespace netx

#endif