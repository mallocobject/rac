#ifndef RAC_ASYNC_CORO_HANDLE_HPP
#define RAC_ASYNC_CORO_HANDLE_HPP

#include "rac/async/event_loop.hpp"
#include "rac/async/handle.hpp"
#include <cstddef>
#include <format>
#include <source_location>
#include <string>
namespace rac
{
struct CoroHandle : public Handle
{
	std::string frameName() const
	{
		const auto& frame_info = frameInfo();
		return std::format("{} at {}:{}", frame_info.function_name(),
						   frame_info.file_name(), frame_info.line());
	}

	virtual void dumpBackTrace(std::size_t depth = 0) const
	{
	}

	void schedule()
	{
		if (state_ == Handle::State::kUnScheduled)
		{
			EventLoop::loop().call_soon(*this);
		}
	}
	void cancel()
	{
		if (state_ == Handle::State::kScheduled)
		{
			EventLoop::loop().cancel_handle(*this);
		}
	}

  private:
	virtual const std::source_location& frameInfo() const
	{
		static const std::source_location frame_info =
			std::source_location::current();
		return frame_info;
	}
};
} // namespace rac

#endif