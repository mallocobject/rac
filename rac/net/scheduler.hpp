#ifndef RAC_NET_SCHEDULER_HPP
#define RAC_NET_SCHEDULER_HPP

#include "elog/logger.h"
#include "rac/async/async_main.hpp"
#include "rac/async/check_error.hpp"
#include "rac/async/event_loop.hpp"
#include "rac/async/scheduled_task.hpp"
#include "rac/async/task.hpp"
#include "rac/meta/lock_free_queue.hpp"
#include <latch>
#include <sys/eventfd.h>
#include <utility>
#include <vector>
namespace rac
{
class Scheduler
{
  public:
	Scheduler() = default;
	Scheduler(Scheduler&&) = delete;
	~Scheduler() = default;

	void push(Task<>&& task)
	{
		task_queue_.push(std::move(task));
	}

	Task<> schedulerLoop(std::latch& start_latch)
	{
		start_latch.count_down();
		while (true)
		{
			co_await wakeup_awaiter_;
			shallow();
			LOG_DEBUG << "scheduler receives tasks, its address is "
					  << static_cast<void*>(this);
			Task<> tmp(nullptr);
			while (task_queue_.pop(tmp))
			{
				// FIXME
				sts_.emplace_back(std::move(tmp));
			}
		}
	}

	void wakeup()
	{
		uint64_t signal = 1;
		ssize_t n = ::write(wakeup_fd, &signal, sizeof(signal));

		if (n != sizeof(signal))
		{
			LOG_ERROR << "writes " << n << " bytes instead of 8";
		}
	}

  private:
	void shallow()
	{
		uint64_t signal = 1;
		ssize_t n = ::read(wakeup_fd, &signal, sizeof(signal));

		if (n != sizeof(signal))
		{
			LOG_ERROR << "reads " << n << " bytes instead of 8";
		}
	}

  private:
	LockFreeQueue<Task<>> task_queue_;
	std::vector<ScheduledTask<Task<>>> sts_;

	int wakeup_fd{checkError(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))};
	Event wakeup_ev_{.fd = wakeup_fd, .flags = Event::kEventRead};
	EventLoop::EventAwaiter wakeup_awaiter_{
		EventLoop::loop().wait_event(wakeup_ev_)};
};
} // namespace rac

#endif