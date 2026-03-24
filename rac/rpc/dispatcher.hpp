#ifndef RAC_RPC_DISPATCHER_HPP
#define RAC_RPC_DISPATCHER_HPP

#include "rac/meta/function_traits.hpp"
#include "rac/net/buffer.hpp"
#include "rac/rpc/serialize_traits.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
namespace rac
{
class RpcDispatcher
{
  public:
	using RpcHandler = std::function<void(Buffer* req_buf, Buffer* res_buf,
										  std::uint32_t limit)>;

	template <typename Func>
	void bind(const std::string& method_name, Func&& func);

	void dispatch(const std::string& method_name, Buffer* req_buf,
				  Buffer* res_buf, std::uint32_t limit)
	{
		if (auto it = handlers_.find(method_name); it != handlers_.end())
		{
			it->second(req_buf, res_buf, limit);
		}
		else
		{
			throw std::runtime_error("RPC Method not found: " + method_name);
		}
	}

  private:
	std::unordered_map<std::string, RpcHandler> handlers_;
};

template <typename Func>
void RpcDispatcher::bind(const std::string& method_name, Func&& func)
{
	using FunctionTraits =
		FunctionTraits<std::decay_t<Func>>; // void(int) -> void(*)(int)
											// no void(&)(int)
	using ArgsTuple = typename FunctionTraits::ArgsTuple;
	using RetType = typename FunctionTraits::RetType;

	handlers_[method_name] = [func = std::forward<Func>(func)](
								 Buffer* req, Buffer* res, std::uint32_t limit)
	{
		ArgsTuple args;
		DeserializeTraits<ArgsTuple>::deserialize(req, &args, limit);

		if constexpr (std::is_void_v<RetType>)
		{
			std::apply(func, args);
		}
		else
		{
			RetType result = std::apply(func, args);
			using RetTuple = std::tuple<RetType>;
			RetTuple ret{result};
			SerializeTraits<RetTuple>::serialize(res, ret);
		}
	};
}
} // namespace rac

#endif