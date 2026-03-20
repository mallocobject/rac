#ifndef ELOG_FIXED_BUFFER_HPP
#define ELOG_FIXED_BUFFER_HPP

#include "elog/context.h"
#include "elog/noncopyable.h"
#include <cassert>
#include <cstddef>
namespace elog
{
template <size_t N> class FixedBuffer : public noncopyable
{
  public:
	using iterator = Context*;

  private:
	Context* ctxs_;
	size_t index_;

  public:
	FixedBuffer() : ctxs_(new Context[N]), index_(0)
	{
	}

	~FixedBuffer()
	{
		delete[] ctxs_;
		ctxs_ = nullptr;
		index_ = 0;
	}

	FixedBuffer(FixedBuffer&& rhs) : ctxs_(rhs.ctxs_), index_(rhs.index_)
	{
		rhs.ctxs_ = nullptr;
		rhs.index_ = 0;
	}

	FixedBuffer& operator=(FixedBuffer&& rhs)
	{
		if (&rhs == this)
		{
			return *this;
		}

		ctxs_ = rhs.ctxs_;
		index_ = rhs.index_;

		rhs.ctxs_ = nullptr;
		rhs.index_ = 0;

		return *this;
	}

	Context* data() const
	{
		return ctxs_;
	}

	size_t capacity() const
	{
		return N;
	}

	bool check() const
	{
		return ctxs_ != nullptr;
	}

	bool empty() const
	{
		return index_ == 0;
	}

	bool full() const
	{
		return index_ == N;
	}

	size_t size() const
	{
		return index_;
	}

	void push(const Context& ctx)
	{
		assert(ctxs_ != nullptr);

		if (!full())
		{
			ctxs_[index_++] = ctx;
		}
	}

	void push(Context&& ctx)
	{
		if (!full())
		{
			ctxs_[index_++] = std::move(ctx);
		}
	}

	void reset()
	{
		index_ = 0;
	}

	iterator begin() const
	{
		return ctxs_;
	}

	iterator end() const
	{
		return ctxs_ + index_;
	}

	Context& operator[](size_t index) const
	{
		return ctxs_[index];
	}
};
} // namespace elog

#endif