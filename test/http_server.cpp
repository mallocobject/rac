#include "netx/async/async_main.hpp"
#include "netx/async/scheduled_task.hpp"
#include "netx/async/task.hpp"
#include "netx/http/request.hpp"
#include "netx/http/response.hpp"
#include "netx/http/router.hpp"
#include "netx/http/session.hpp"
#include "netx/meta/reflection.hpp"
#include "netx/net/socket.hpp"
#include "netx/net/stream.hpp"
#include "netx/rpc/dispatcher.hpp"
#include "netx/rpc/header.hpp"
#include "netx/rpc/serialize_traits.hpp"
#include <any>
#include <csignal>
#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <tuple>

using namespace netx;
using namespace netx::http;
using namespace netx::net;
using namespace netx::async;

HttpRouter router{};

Task<> handle_client(int fd)
{
	Stream s{fd};
	Session session{};

	while (true)
	{

		if (!session.parse(s.read_buffer()))
		{
			co_await s.write("HTTP/1.1 400 Bad Request\r\n\r\n");
			s.close();
			co_return;
		}

		if (session.completed())
		{
			const http::HttpRequest& req = session.req();
			http::HttpResponse res;

			co_await router.dispatch(req, &res, &s);

			std::string conn_header = req.header("connection");
			if (conn_header == "close" ||
				(req.version == "HTTP/1.0" && conn_header != "keep-alive"))
			{
				s.close();
				co_return;
			}
			else
			{
				session.clear();
			}
		}

		auto rd_buf = co_await s.read();
		if (!rd_buf)
		{
			s.close();
			co_return;
		}
	}
}

Task<> server_loop(int listen_fd)
{
	router.route(
		"GET", "/",
		[](const HttpRequest& req, HttpResponse* res, Stream* stream) -> Task<>
		{
			res->set_status(200);
			res->set_content_type("text/html");
			res->set_body("<h1>Hello Netx</h1>");

			co_await stream->write(res->to_formatted_string());
		});

	std::list<ScheduledTask<Task<>>> connections;
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

			connections.emplace_back(handle_client(conn_fd));
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

int main()
{
	::signal(SIGPIPE, SIG_IGN);

	int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	InetAddr addr{8080, true};

	Socket::bind(listen_fd, addr, nullptr);
	Socket::listen(listen_fd, nullptr);

	LOG_WARN << "Server listening on http://127.0.0.1:8080";

	async_main(server_loop(listen_fd));
}