#ifndef ELOG_ASYNCLOGGING_H
#define ELOG_ASYNCLOGGING_H

#include "elog/context.h"
#include "elog/fixed_buffer.hpp"
#include "elog/formatter.h"
#include "elog/noncopyable.h"
#include <atomic>
#include <condition_variable>
#include <latch>
#include <mutex>
#include <thread>
#include <vector>

namespace elog
{
enum
{
	kSmallBuffer = 4096,
	kLargeBuffer = 65536,
};

class AsyncLogging : public noncopyable
{
  private:
	using Buffer = FixedBuffer<kLargeBuffer>;

  private:
	const int flush_interval_;
	const int roll_size_;
	std::atomic<bool> done_;
	const std::string basename_;
	const std::string prefix_;
	std::mutex mtx_;
	std::condition_variable cv_;
	std::latch latch_down_;

	std::thread thread_;

	Formatter formatter_;
	Buffer cur_buf_;
	Buffer next_buf_;
	std::vector<Buffer> bufs_;

  public:
	explicit AsyncLogging(const std::string& basename,
						  const std::string& prefix, int roll_size,
						  int flush_interval);

	~AsyncLogging();

	void pushMessage(const Context& ctx);
	void wait4Done();

  private:
	void doDone();
	void threadWorker();
};
} // namespace elog

#endif