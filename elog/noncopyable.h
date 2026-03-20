#ifndef ELOG_NONCOPYABLE_H
#define ELOG_NONCOPYABLE_H

namespace elog
{
class noncopyable
{
  protected:
	noncopyable() = default;
	~noncopyable() = default;

	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};
} // namespace elog

#endif