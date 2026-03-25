#include "rac/rpc/server.hpp"
#include <csignal>
#include <string>

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
	server.bind("EchoUser", [](const User& usr) { return usr; })
		.bind("AddPoint",
			  [](Point a, Point b) { return Point{a.x + b.x, a.y + b.y}; })
		.loop(4)
		.start();
}