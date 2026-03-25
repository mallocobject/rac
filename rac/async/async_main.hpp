#ifndef RAC_ASYNC_ASYNC_MAIN_HPP
#define RAC_ASYNC_ASYNC_MAIN_HPP

#include "rac/async/concepts.hpp"
#include "rac/async/event_loop.hpp"
#include "rac/async/scheduled_task.hpp"
#include <utility>
namespace rac
{
template <Future Fut> decltype(auto) async_main(Fut&& main)
{
	auto task = ScheduledTask(std::move(main));
	EventLoop::loop().run_until_complete();
	return task.result();
}

inline void async_main()
{
	EventLoop::loop().run_until_complete();
}
} // namespace rac

#endif