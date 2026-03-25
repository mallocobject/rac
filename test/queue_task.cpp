#include "rac/async/async_main.hpp"
#include "rac/async/scheduled_task.hpp"
#include "rac/async/task.hpp"
#include "rac/meta/intrusive_list.hpp"
#include "rac/meta/lock_free_queue.hpp"
#include <thread>
#include <utility>

using namespace rac;

Task<> task()
{
	std::cout << "hello" << std::endl;

	co_return;
}

int main()
{
	LockFreeQueue<Task<>> q;

	std::thread prod([&] { q.push(task()); });
	prod.join();

	Intrusive_list<ScheduledTask<Task<>>> l;
	std::thread con(
		[&]
		{
			Task<> tmp(nullptr);
			q.pop(tmp);
			ScheduledTask<Task<>> st(std::move(tmp));
			l.insert(st);
			std::cout << l.size() << std::endl;
			async_main();
		});

	con.join();
	std::cout << l.size() << std::endl;
}