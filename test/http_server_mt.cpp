#include "netx/http/server.hpp"
#include <csignal>
#include <string>

using namespace netx::http;
using namespace netx::net;
using namespace netx::async;

int main()
{
	HttpServer::server()
		.listen("127.0.0.1", 8080)
		.route(
            "GET", "/",
            [](const HttpRequest& req, HttpResponse* res, Stream* stream) -> Task<>
            {
                res->set_status(200);
                res->set_content_type("text/html");
                res->set_body("<h1>Hello Netx</h1>");

                co_await stream->write(res->to_formatted_string());
            })
		.loop(8)
		.start();
}