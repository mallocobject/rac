#ifndef NETX_HTTP_SERVER_HPP
#define NETX_HTTP_SERVER_HPP

#include "netx/http/router.hpp"
#include "netx/http/session.hpp"
#include "netx/net/server.hpp"
#include "netx/net/stream.hpp"
#include "netx/async/sleep.hpp"
#include "netx/async/when_any.hpp"
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

    try {
        while (true)
        {
            // s.read() 返回 Task<bool>，true 表示读到了数据，false 表示 EOF（客户端关闭）
            // async::sleep() 返回 Task<void>
            auto ret = co_await async::when_any(s.read(), async::sleep(timeout_));

            // 索引 0 是 s.read() 的结果 (bool)
            // 索引 1 是 sleep() 的结果 (NonVoidHelper<void>)
            if (ret.index() == 1) 
            {
                elog::LOG_WARN("Connection timeout on fd: {}", s.fd());
                co_await s.write("HTTP/1.1 408 Request Timeout\r\nConnection: close\r\n\r\n");
                break; 
            }

            bool read_ok = std::get<0>(ret);
            if (!read_ok) 
            {
                break;
            }

            while (s.read_buffer()->readableBytes() > 0)
            {
                if (!session.parse(s.read_buffer()))
                {
                    co_await s.write("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
                    co_return;
                }

                if (session.completed())
                {
                    const http::HttpRequest& req = session.req();
                    http::HttpResponse res;

                    co_await router_.dispatch(req, &res, &s);

                    std::string conn_header = req.header("connection");
                    bool keep_alive = true;
                    if (conn_header == "close" ||
                        (req.version == "HTTP/1.0" && conn_header != "keep-alive"))
                    {
                        keep_alive = false;
                    }

                    if (!keep_alive)
                    {
                        s.shutdown();
                        co_return;
                    }

                    session.clear();
                }
                else 
                {
                    break; 
                }
            }
        }
    }
    catch (const std::exception& e) {
        elog::LOG_ERROR("Exception in handleClient: {}", e.what());
    }

    co_return;
}
} // namespace http
} // namespace netx

#endif