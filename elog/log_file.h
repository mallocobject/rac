#ifndef ELOG_LOG_FILE_H
#define ELOG_LOG_FILE_H

#include "elog/noncopyable.h"
#include <memory>
#include <string>
namespace elog
{
class FileAppender;
class LogFile : public noncopyable
{
  private:
	const std::string basename_;
	const std::string prefix_;
	const int roll_size_;
	const int flush_interval_;
	const int check_per_count_;
	int count_;

	std::unique_ptr<FileAppender> file_;

	time_t last_day_;
	time_t last_roll_second_;
	time_t last_flush_second_;

  public:
	explicit LogFile(const std::string& basename, const std::string& prefix,
					 int roll_size, int flush_interval,
					 int check_per_count = 1024);

	~LogFile();

	void append(const char* data, size_t len);
	void flush();

  private:
	void appendWOLock(const char* data, size_t len);
	void rollFile(const time_t* cached_now = nullptr);
	std::string getFilename(const time_t& now);
};
} // namespace elog

#endif