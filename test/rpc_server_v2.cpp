#include "rac/async/async_main.hpp"
#include "rac/async/scheduled_task.hpp"
#include "rac/async/task.hpp"
#include "rac/meta/reflection.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include "rac/rpc/dispatcher.hpp"
#include "rac/rpc/rpc_header.hpp"
#include "rac/rpc/serialize_traits.hpp"
#include "rac/rpc/server.hpp"
#include <csignal>
#include <cstddef>
#include <list>
#include <string>
#include <tuple>

using namespace rac;

struct Point
{
	double x;
	double y;
};

struct User
{
	std::uint32_t id;
	std::string name;
	Point loc;
};

int main()
{
	RpcServer server("127.0.0.1", 8080);
	server.bind("EchoUser", [](const User& usr) { return usr; });

	server.bind("AddPoint",
				[](Point a, Point b) { return Point{a.x + b.x, a.y + b.y}; });

	server.start();
}