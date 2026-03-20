#ifndef RAC_ASYNC_SLEEP_HPP
#define RAC_ASYNC_SLEEP_HPP

#include "rac/async/concepts.hpp"
#include "rac/async/event_loop.hpp"
#include "rac/async/task.hpp"
#include <chrono>
namespace rac
{

template <typename Duration> struct SleepAwaiter
{
	explicit SleepAwaiter(Duration duration) : duration(duration)
	{
	}

	bool await_ready() const noexcept
	{
		return false;
	}

	template <Promise P>
	void await_suspend(std::coroutine_handle<P> coro) const noexcept
	{
		EventLoop::loop().call_after(duration, coro.promise());
	}

	void await_resume() const noexcept
	{
	}

  private:
	Duration duration;
};

template <typename Rep, typename Period>
[[nodiscard]] Task<> sleep(NoWaitAtInitialSuspend,
						   std::chrono::duration<Rep, Period> duration)
{
	co_await SleepAwaiter{duration};
}

template <typename Rep, typename Period>
[[nodiscard]] Task<> sleep(std::chrono::duration<Rep, Period> duration)
{
	return sleep(no_wait_at_initial_suspend, duration);
}

} // namespace rac

#endif