#ifndef RAC_ASYNC_NON_VOID_HELPER_HPP
#define RAC_ASYNC_NON_VOID_HELPER_HPP

namespace rac
{
template <typename T = void> struct NonVoidHelper
{
	using Type = T;
};

template <> struct NonVoidHelper<void>
{
	using Type = NonVoidHelper;

	explicit NonVoidHelper() = default;
};
} // namespace rac

#endif
