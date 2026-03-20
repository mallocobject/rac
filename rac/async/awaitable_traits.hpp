#ifndef RAC_ASYNC_AWAITABLE_TRAITS_HPP
#define RAC_ASYNC_AWAITABLE_TRAITS_HPP

#include "rac/async/concepts.hpp"
#include "rac/async/non_void_helper.hpp"
namespace rac
{
template <typename A> struct AwaitableTraits;

template <Awaiter A> struct AwaitableTraits<A>
{
    using RetType = decltype(std::declval<A>().await_resume());
    using NonVoidRetType = NonVoidHelper<RetType>::Type;
};

template <typename A>
    requires(!Awaiter<A> && Awaitable<A>)
struct AwaitableTraits<A> : AwaitableTraits<decltype(std::declval<A>().operator co_await())>
{
};
} // namespace rac

#endif