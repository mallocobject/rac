#ifndef ELOG_CURRENT_THREAD_H
#define ELOG_CURRENT_THREAD_H

#include <cstdint>
#include <thread>
namespace elog
{
namespace CurrentThread
{
inline uint64_t tid()
{
	thread_local uint64_t cached_tid =
		std::hash<std::thread::id>{}(std::this_thread::get_id());
	return cached_tid + 1;
}

inline std::string& str()
{
	thread_local std::string str;
	static bool initialed = []()
	{
		str.reserve(128);
		return true;
	}();

	return str;
}
} // namespace CurrentThread
} // namespace elog

#endif