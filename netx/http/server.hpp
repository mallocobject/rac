#ifndef NETX_HTTP_SERVER_HPP
#define NETX_HTTP_SERVER_HPP

#include "netx/http/router.hpp"
#include "netx/http/session.hpp"
#include "netx/net/server.hpp"
#include "netx/net/stream.hpp"
namespace netx
{
namespace http
{
namespace async = netx::async;
namespace net = netx::net;
class HttpServer : public net::Server<HttpServer>
{
	friend class net::Server<HttpServer>;
	HttpServer() = default;

  public:
	static HttpServer& server()
	{
		static HttpServer http_server{};
		return http_server;
	}

	template <typename Handler>
	HttpServer& route(const std::string& method, const std::string& path,
					  Handler&& handler)
	{
		router_.route(method, path, std::forward<Handler>(handler));
		return *this;
	}

  private:
	async::Task<> handleClient(int conn_fd);

	// async::Task<> serverLoop();

  private:
	HttpRouter router_{};
};

inline async::Task<> HttpServer::handleClient(int conn_fd)
{
	net::Stream s{conn_fd};
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

			co_await router_.dispatch(req, &res, &s);

			std::string conn_header = req.header("connection");
			if (conn_header == "close" ||
				(req.version == "HTTP/1.0" && conn_header != "keep-alive"))
			{
				s.shutdown();
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
} // namespace http
} // namespace netx

#endif