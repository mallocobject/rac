#include "elog/log_file.h"
#include "elog/file_appender.h"
#include <cstdint>
#include <ctime>
#include <format>
#include <memory>

using namespace elog;

static const int kRollPerSeconds = 24 * 60 * 60;

LogFile::LogFile(const std::string& basename, const std::string& prefix,
				 int roll_size, int flush_interval, int check_per_count)
	: basename_(basename), prefix_(prefix), roll_size_(roll_size),
	  flush_interval_(flush_interval), check_per_count_(check_per_count),
	  count_(0)
{
	rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* data, size_t len)
{
	appendWOLock(data, len);
}

void LogFile::flush()
{
	file_->flush();
}

void LogFile::appendWOLock(const char* data, size_t len)
{
	file_->append(data, len);
	if (file_->writtenBytes() > roll_size_)
	{
		rollFile();
		file_->resetWrittenBytes();
	}
	else
	{
		count_++;
		if (count_ >= check_per_count_)
		{
			count_ = 0;
			int64_t now = ::time(nullptr);
			int64_t cur_day = now / kRollPerSeconds * kRollPerSeconds;
			if (cur_day != last_day_)
			{
				rollFile(&now);
			}
			else if (now - last_flush_second_ >= flush_interval_)
			{
				last_flush_second_ = now;
				file_->flush();
			}
		}
	}
}

void LogFile::rollFile(const time_t* cached_now)
{
	time_t now = cached_now ? *cached_now : ::time(nullptr);

	std::string filename = getFilename(now);
	auto now_day = now / kRollPerSeconds * kRollPerSeconds;

	if (now > last_roll_second_)
	{
		last_roll_second_ = now;
		last_flush_second_ = now;
		last_day_ = now_day;

		file_ = std::make_unique<FileAppender>(filename);
	}
}

std::string LogFile::getFilename(const time_t& now)
{
	std::tm tm;
	::localtime_r(&now, &tm);

	return std::format(
		"{0}/{1}.{2:04d}{3:02d}{4:02d}-{5:02d}{6:02d}{7:02d}.log", basename_,
		prefix_, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		tm.tm_min, tm.tm_sec);
}