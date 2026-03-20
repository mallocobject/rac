#ifndef RAC_ASYNC_HANDLE_HPP
#define RAC_ASYNC_HANDLE_HPP

#include <cstdint>
namespace rac
{
using HandleId = std::uint64_t;

struct Handle
{
	enum class State : std::uint8_t
	{
		kUnScheduled,
		kSuspend,
		kScheduled,
	};

	Handle() noexcept : handle_id_(handle_id_generation_++)
	{
	}

	virtual ~Handle() = default;

	virtual void run() = 0;

	void setState(State state)
	{
		state_ = state;
	}

	HandleId handleId()
	{
		return handle_id_;
	}

  private:
	HandleId handle_id_;
	inline static HandleId handle_id_generation_ = 0;

  protected:
	State state_{Handle::State::kUnScheduled};
};
} // namespace rac

#endif