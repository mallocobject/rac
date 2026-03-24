#include "elog/logger.h"
#include "rac/async/async_main.hpp"
#include "rac/async/task.hpp"
#include "rac/net/inet_addr.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include "rac/rpc/client.hpp"
#include "rac/rpc/rpc_header.hpp"
#include "rac/rpc/serialize_traits.hpp"
#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <thread>

using namespace rac;
using namespace std::chrono_literals;

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

	std::string email;
	std::uint64_t phone_number;
};

Task<> connect_server(RpcClient client)
{
	struct ScopeGuard
	{
		RpcClient& c;
		~ScopeGuard()
		{
			c.close();
		}
	} guard{client};

	try
	{
		while (true)
		{
			auto res = co_await client.call<Point>("AddPoint", Point{1, 2},
												   Point{5.2, 6.8});
			std::cout << "x: " << res.x << std::endl;
			std::cout << "y: " << res.y << std::endl;

			std::this_thread::sleep_for(1s);

			auto usr = co_await client.call<User>("EchoUser",
												  User{8, "liuna", {1, 5}});

			std::cout << "ID: " << usr.id << "\n";
			std::cout << "Name: " << usr.name << "\n";
			std::cout << "Location: (" << usr.loc.x << ", " << usr.loc.y
					  << ")\n";

			std::this_thread::sleep_for(1s);
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR << e.what();
	}

	// client.close();
}

int main()
{
	async_main(connect_server(RpcClient{"127.0.0.1", 8080}));
}