#ifndef ELOG_TIMESTAMP_H
#define ELOG_TIMESTAMP_H

#include <chrono>
#include <ostream>
#include <string>
namespace elog
{
struct Timestamp
{
	using Clock = std::chrono::system_clock;
	using TimePoint = std::chrono::time_point<Clock, std::chrono::microseconds>;

	TimePoint tp;

	explicit Timestamp(
		TimePoint t =
			std::chrono::time_point_cast<TimePoint::duration>(Clock::now()))
		: tp(t)
	{
	}

	std::strong_ordering operator<=>(const Timestamp& other) const
	{
		return tp <=> other.tp;
	}

	bool operator==(const Timestamp& other) const
	{
		return (*this <=> other) == 0;
	}

	static Timestamp now()
	{
		return Timestamp();
	}

	std::string toFormattedString(bool date = true, bool time = true) const;

	friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts);
};

std::ostream& operator<<(std::ostream& os, const Timestamp& ts);
} // namespace elog

#endif