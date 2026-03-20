#include "elog/async_logging.h"
#include "elog/context.h"
#include "elog/log_file.h"
#include "elog/timestamp.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <format>
#include <functional>
#include <iostream>
#include <latch>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

using namespace elog;

AsyncLogging::AsyncLogging(const std::string& basename,
						   const std::string& prefix, int roll_size,
						   int flush_interval)
	: flush_interval_(flush_interval), roll_size_(roll_size), done_(false),
	  basename_(basename), prefix_(prefix), latch_down_(1)
{
	thread_ = std::thread(std::bind(&AsyncLogging::threadWorker, this));
	latch_down_.wait();
}

AsyncLogging::~AsyncLogging()
{
	if (done_.load(std::memory_order_acquire))
	{
		return;
	}
	doDone();
}

void AsyncLogging::pushMessage(const Context& ctx)
{
	if (done_.load(std::memory_order_acquire))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(mtx_);
	if (!cur_buf_.full())
	{
		cur_buf_.push(ctx);
		return;
	}

	bufs_.push_back(std::move(cur_buf_));
	if (next_buf_.check())
	{
		cur_buf_ = std::move(next_buf_);
	}
	else
	{
		cur_buf_ = Buffer();
	}

	cur_buf_.push(ctx);
	cv_.notify_one();
}

void AsyncLogging::doDone()
{
	done_.store(true, std::memory_order_release);
	cv_.notify_one();

	if (thread_.joinable())
	{
		thread_.join();
	}
}

void AsyncLogging::wait4Done()
{
	if (done_.load(std::memory_order_acquire))
	{
		return;
	}
	doDone();
}

void AsyncLogging::threadWorker()
{
	LogFile out_file(basename_, prefix_, roll_size_, flush_interval_);
	Buffer new_buf1;
	Buffer new_buf2;

	std::vector<Buffer> buf2write;
	buf2write.reserve(16); // only change its capacity

	while (!done_.load(std::memory_order_acquire))
	{
		{
			std::unique_lock<std::mutex> lock(mtx_);
			if (bufs_.empty())
			{
				static std::once_flag flag;
				std::call_once(flag, [&]() { latch_down_.count_down(); });
				cv_.wait_for(lock, std::chrono::seconds(flush_interval_));
			}

			if (!cur_buf_.empty())
			{
				bufs_.push_back(std::move(cur_buf_));
				cur_buf_ = std::move(new_buf1);
			}

			buf2write.swap(bufs_);

			if (!next_buf_.check())
			{
				next_buf_ = std::move(new_buf2);
			}
		}

		if (buf2write.empty())
		{
			continue;
		}

		if (buf2write.size() > 100)
		{
			// 保留两个
			std::string err = std::format(
				"Dropped log messages at {}, {} larger buffers\n",
				Timestamp::now().toFormattedString(), buf2write.size() - 2);
			std::cerr << err;

			out_file.append(err.c_str(), err.size());
			buf2write.erase(buf2write.begin() + 2, buf2write.end());
		}

		for (auto& buffer : buf2write)
		{
			for (auto& ctx : buffer)
			{
				std::string formatted_log = formatter_.format(ctx);
				out_file.append(formatted_log.c_str(), formatted_log.size());
			}
		}

		if (buf2write.size() < 2)
		{
			buf2write.resize(2);
		}

		if (!new_buf1.check())
		{
			new_buf1 = std::move(buf2write[0]);
			new_buf1.reset();
		}

		if (!new_buf2.check())
		{
			new_buf2 = std::move(buf2write[1]);
			new_buf2.reset();
		}

		buf2write.clear();
		out_file.flush();
	}

	out_file.flush();
}