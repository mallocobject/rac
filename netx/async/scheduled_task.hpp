#ifndef NETX_ASYNC_SCHEDULED_TASK_HPP
#define NETX_ASYNC_SCHEDULED_TASK_HPP

#include "netx/async/concepts.hpp"
#include "netx/async/coro_handle.hpp"
#include <list>
#include <utility>
namespace netx
{
namespace async
{
template <Future TaskT> struct ScheduledTask
{
	struct DeleteNodeHandle : public CoroHandle
	{
		std::list<ScheduledTask>* owner;
		typename std::list<ScheduledTask>::iterator iter;

		DeleteNodeHandle(std::list<ScheduledTask>* o,
						 typename std::list<ScheduledTask>::iterator i)
			: owner(o), iter(i)
		{
		}

		void run() override
		{
			if (owner && iter != owner->end())
			{
				owner->erase(iter);
			}
			delete this; // 666
		}
	};

	// template <Future Fut>
	// explicit ScheduledTask(Fut&& fut) noexcept :
	// task_(std::forward<Fut>(fut))
	// {
	// 	if (task_.valid() && !task_.done())
	// 	{
	// 		task_.coro_.promise().schedule();
	// 	}
	// }

	// explicit ScheduledTask(const TaskT& fut) noexcept : task_(fut)
	// {
	// 	if (task_.valid() && !task_.done())
	// 		task_.coro_.promise().schedule();
	// }

	ScheduledTask() noexcept = default;

	explicit ScheduledTask(TaskT&& fut) noexcept : task_(std::move(fut))
	{
		if (task_.valid() && !task_.done())
		{
			task_.coro_.promise().schedule();
		}
	}

	ScheduledTask(TaskT&& fut, std::list<ScheduledTask>* owner,
				  typename std::list<ScheduledTask>::iterator iter)
		: task_(std::move(fut)), owner_(owner), iter_(iter)
	{
		if (task_.valid())
		{
			if (!task_.done())
			{
				auto* del_handle = new DeleteNodeHandle(owner, iter);
				task_.coro_.promise().continuation = del_handle;
				task_.coro_.promise().schedule();
			}
			else
			{
				if (owner && iter != owner->end())
				{
					owner->erase(iter);
				}
			}
		}
	}

	ScheduledTask(ScheduledTask&& other) noexcept
		: task_(std::move(other.task_)), owner_(other.owner_),
		  iter_(other.iter_)
	{
		other.owner_ = nullptr;
	}

	ScheduledTask& operator=(ScheduledTask&& other) noexcept
	{
		if (this == &other)
		{
			*this;
		}
		task_ = std::move(other.task_);
		owner_ = other.owner_;
		iter_ = other.iter_;
		other.owner_ = nullptr;
		return *this;
	}

	// ScheduledTask()
	// {
	// }

	~ScheduledTask() = default;

	decltype(auto) result() &
	{
		return task_.result();
	}

	decltype(auto) result() &&
	{
		return std::move(task_).result();
	}

	auto operator co_await() const& noexcept
	{
		return task_.operator co_await();
	}

	auto operator co_await() && noexcept
	{
		return std::move(task_).operator co_await();
	}

	bool valid() const
	{
		return task_.valid();
	}
	bool done() const
	{
		return task_.done();
	}

	void cancel()
	{
		task_.destroy();
	}

  private:
	TaskT task_{nullptr};
	std::list<ScheduledTask>* owner_;
	typename std::list<ScheduledTask>::iterator iter_;
};

// template <Future Fut> ScheduledTask(Fut&&) -> ScheduledTask<Fut>;

template <Future Fut>
[[nodiscard(
	"discard(detached) a task will not schedule to run")]] ScheduledTask<Fut>
co_spawn(Fut&& fut) // 内部无 await，需要 co_await 延长生命周期
{
	return ScheduledTask<Fut>(std::move(fut));
}
} // namespace async
} // namespace netx

#endif