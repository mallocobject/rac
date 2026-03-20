#ifndef ELOG_FLAGS_H
#define ELOG_FLAGS_H

#include <cstdint>

namespace elog
{
class Flags
{
  public:
	enum Value : uint8_t
	{
		kDate = 1 << 0,
		kTime = 1 << 1,
		kFullName = 1 << 2,
		kShortName = 1 << 3,
		kLine = 1 << 4,
		kFuncName = 1 << 5,
		kThreadId = 1 << 6,
		kStdFlags = kDate | kTime | kThreadId | kShortName | kLine | kFuncName
	};

  private:
	uint8_t value_{0};

  public:
	constexpr Flags() = default;
	constexpr Flags(Value v) : value_(v)
	{
	}

	constexpr Flags operator|(Flags other) const
	{
		return Flags(static_cast<Value>(value_ | other.value_));
	}

	constexpr Flags operator&(Flags other) const
	{
		return Flags(static_cast<Value>(value_ & other.value_));
	}

	constexpr Flags operator&(uint8_t other) const
	{
		return Flags(static_cast<Value>(value_ & other));
	}

	constexpr Flags operator^(Flags other) const
	{
		return Flags(static_cast<Value>(value_ ^ other.value_));
	}

	constexpr Flags operator~() const
	{
		return Flags(static_cast<Value>(~value_));
	}

	constexpr Flags& operator|=(Flags other)
	{
		value_ = static_cast<Value>(value_ | other.value_);
		return *this;
	}

	constexpr Flags& operator&=(Flags other)
	{
		value_ = static_cast<Value>(value_ & other.value_);
		return *this;
	}

	constexpr Flags& operator^=(Flags other)
	{
		value_ = static_cast<Value>(value_ ^ other.value_);
		return *this;
	}

	// 转换操作符 - 支持 if (flags & Flags::kTime) 语法
	constexpr explicit operator bool() const
	{
		return value_ != 0;
	}

	constexpr uint8_t value() const
	{
		return value_;
	}
};
} // namespace elog

#endif