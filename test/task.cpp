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

Task<int> sleep1()
{
	co_await sleep(2s);
	LOG_INFO << "sleep1苏醒";
	co_return 1;
}

Task<int> sleep2()
{
	co_await sleep(5s);
	LOG_INFO << "sleep2苏醒";
	co_return 2;
}

Task<int> sleep_()
{
	LOG_INFO << "添加定时器1";
	auto t2 = co_spawn(sleep2()); // 5s
	LOG_INFO << "添加定时器2";
	auto t1 = co_spawn(sleep1()); // 2s

	LOG_INFO << "等待定时器1";
	int ret = 0;
	ret += co_await t1; // 不挂起当函数协程执行完，析构对象的同时会cancel handle
	LOG_INFO << "等待定时器2";

	t2.cancel();

	co_return ret;
}

int main()
{
	auto start = std::chrono::steady_clock::now();
	LOG_WARN << async_main(sleep_());

	LOG_INFO << std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::steady_clock::now() - start)
					.count();
}