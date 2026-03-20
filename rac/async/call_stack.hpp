#ifndef RAC_ASYNC_CALL_STACK_HPP
#define RAC_ASYNC_CALL_STACK_HPP

#include "rac/async/concepts.hpp"
#include <coroutine>
namespace rac
{
struct CallStackAwaiter
{
	bool await_ready() const noexcept
	{
		return false;
	}

	template <Promise P>
	bool await_suspend(std::coroutine_handle<P> handle) const noexcept
	{
		handle.promise().dumpBackTrace();
		return false;
	}

	void await_resume() const noexcept
	{
	}
};

[[nodiscard]] inline CallStackAwaiter dump_call_stack()
{
	return {};
}
} // namespace rac

#endif