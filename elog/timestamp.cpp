#include "elog/timestamp.h"
#include <chrono>
#include <ctime>
#include <format>

using namespace elog;

std::string Timestamp::toFormattedString(bool date, bool time) const
{
	if (!date && !time)
	{
		return "";
	}

	auto us = tp.time_since_epoch().count();
	auto secs = us / 1'000'000;
	auto micros = us % 1'000'000;

	tm tm_time;
	::localtime_r(&secs, &tm_time);

	if (date && time)
	{
		return std::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:06d}",
						   tm_time.tm_year + 1900, tm_time.tm_mon + 1,
						   tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min,
						   tm_time.tm_sec, micros);
	}
	else if (date)
	{
		return std::format("{:04d}-{:02d}-{:02d}", tm_time.tm_year + 1900,
						   tm_time.tm_mon + 1, tm_time.tm_mday);
	}

	return std::format("{:02d}:{:02d}:{:02d}.{:06d}", tm_time.tm_hour,
					   tm_time.tm_min, tm_time.tm_sec, micros);
}

std::ostream& operator<<(std::ostream& os, const Timestamp& ts)
{
	os << ts.toFormattedString();
	return os;
}