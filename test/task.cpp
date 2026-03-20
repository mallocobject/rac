#include "rac/async/task.hpp"
#include "elog/logger.h"
#include "rac/async/async_main.hpp"
#include "rac/async/call_stack.hpp"
#include "rac/async/scheduled_task.hpp"
#include "rac/async/sleep.hpp"
#include <chrono>

using namespace rac;
using namespace std::chrono_literals;

Task<int> world()
{
	std::cout << "World" << std::endl;
	co_await dump_call_stack();
	co_return 2;
}

Task<int> hello()
{
	std::cout << "Hello" << std::endl;
	auto a = co_await world();
	std::cout << "result :" << a << std::endl;

	co_return a;
}

Task<> sleep1()
{
	co_await sleep(2s);
}

Task<> sleep2()
{
	co_await sleep(5s);
}

Task<> sleep_()
{
	co_await sleep1();
	LOG_INFO << "sleep1苏醒";
	co_await sleep2();
	LOG_INFO << "sleep2苏醒";
}

int main()
{
	auto start = std::chrono::steady_clock::now();
	auto t = co_spawn(sleep_());
	async_main(hello());

	LOG_INFO << std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::steady_clock::now() - start)
					.count();
}