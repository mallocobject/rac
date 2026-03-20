#include "elog/file_appender.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

using namespace elog;

static constexpr size_t BUFFER_SIZE = 64 * 1024;

namespace
{
void throw_system_error(std::string operation)
{
	throw std::system_error(errno, std::system_category(), operation.c_str());
}

void throw_runtime_error(std::string message)
{
	throw std::runtime_error(message);
}
} // namespace

FileAppender::FileAppender(const std::filesystem::path& filepath)
	: filepath_(filepath), buf_(new char[BUFFER_SIZE])
{
	ensureDirectoryExists();

	file_ = ::fopen(filepath.c_str(), "ab");
	if (!file_)
	{
		throw_system_error("Failed to open log file");
	}

	// if (::setvbuf(file_, buf_, _IOFBF, BUFFER_SIZE) != 0)
	// {
	// 	::fclose(file_);
	// 	file_ = nullptr;
	// 	throw_runtime_error("Failed to set file buffer");
	// }
}

FileAppender::~FileAppender()
{
	if (file_)
	{
		::fflush(file_);
		::fclose(file_);
		file_ = nullptr;
	}
	if (buf_)
	{
		delete[] buf_;
		buf_ = nullptr;
	}
}

void FileAppender::append(const char* data, size_t len)
{
	if (!data || !file_ || len == 0)
	{
		return;
	}

	const size_t written = ::fwrite_unlocked(data, 1, len, file_);

	if (written != len)
	{
		if (::ferror(file_))
		{
			throw_system_error("Failed to write log file");
		}
		throw_runtime_error("Partial write to log file");
	}

	written_bytes_ += len;
}

void FileAppender::flush()
{
	if (!file_)
	{
		return;
	}

	if (::fflush(file_) != 0)
	{
		throw_system_error("Failed to flush log file");
	}
}

void FileAppender::ensureDirectoryExists() const
{
	const auto parent_path = filepath_.parent_path();

	if (parent_path.empty())
	{
		// relative path
		return;
	}

	std::error_code ec;

	auto status = std::filesystem::status(parent_path, ec);

	if (ec)
	{
		throw_system_error("Failed to check directory: " +
						   std::string(parent_path));
	}

	if (!std::filesystem::exists(status))
	{
		throw_runtime_error("Directory does not exist: " +
							parent_path.string());
	}

	if (!std::filesystem::is_directory(status))
	{
		throw_runtime_error("Path exists but is not a directory: " +
							parent_path.string());
	}
}