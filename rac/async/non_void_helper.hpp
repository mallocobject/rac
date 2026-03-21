#ifndef RAC_ASYNC_NON_VOID_HELPER_HPP
#define RAC_ASYNC_NON_VOID_HELPER_HPP

#include <type_traits>
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

template <typename T> constexpr bool is_non_void(T)
{
	using T_raw = std::remove_cvref_t<T>;
	return !std::is_same_v<T_raw, NonVoidHelper<>>;
}
} // namespace rac

#endif
