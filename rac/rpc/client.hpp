#ifndef RAC_RPC_CLIENT_HPP
#define RAC_RPC_CLIENT_HPP

#include "elog/logger.h"
#include "rac/async/check_error.hpp"
#include "rac/net/inet_addr.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include "rac/rpc/serialize_traits.hpp"
#include <string>
#include <utility>
namespace rac
{
// RpcClient client("127.0.0.1", 8080);
// User result = client.call<User>("Echo", User{123, "Alice", {1,2}});

class RpcClient
{
  public:
	explicit RpcClient(const InetAddr& sock_addr)
		: stream_(checkError(Socket::socket(nullptr)), sock_addr)
	{
		while (!Socket::connect(stream_.fd(), sock_addr, nullptr))
		{
		}
	}

	explicit RpcClient(const std::string& ip, std::uint16_t port)
		: RpcClient(InetAddr{ip, port})
	{
	}

	RpcClient(RpcClient&& other) noexcept : stream_(std::move(other.stream_))
	{
	}

	void close()
	{
		stream_.close();
	}

	template <typename Ret, typename... Args>
	Task<Ret> call(const std::string& method_name, Args&&... args)
	{
		std::size_t size_before = stream_.write_buffer()->readableBytes();
		SerializeTraits<std::string>::serialize(stream_.write_buffer(),
												method_name);
		using ArgsType = std::tuple<std::remove_cvref_t<Args>...>;
		ArgsType args_tuple{std::forward<Args>(args)...};
		SerializeTraits<ArgsType>::serialize(stream_.write_buffer(),
											 args_tuple);

		RpcHeader h{.magic = kMagic,
					.version = kVersion,
					.flags = 1,
					.header_len = kRpcHeaderWireLength,
					.body_len = static_cast<std::uint32_t>(
						stream_.write_buffer()->readableBytes() - size_before),
					.request_id = 0,
					.reserved = 0};
		stream_.write_buffer()->prependRpcHeader(h);

		bool write_success = co_await stream_.write();
		if (!write_success)
		{
			LOG_ERROR << "Failed to write to server on fd " << stream_.fd()
					  << ", connection might be dead. Method: " << method_name;
			throw std::runtime_error(
				"Failed to write to server, connection might be dead.");
		}

		while (stream_.read_buffer()->readableBytes() < sizeof(RpcHeaderWire))
		{
			auto rd_buf = co_await stream_.read();
			if (!rd_buf)
			{
				LOG_ERROR
					<< "Connection closed by peer before reading header in fd "
					<< stream_.fd() << ". Method: " << method_name;
				throw std::runtime_error(
					"Server connection lost while reading header.");
			}
		}

		h = stream_.read_buffer()->peekRpcHeader();
		std::size_t total_len = h.header_len + h.body_len;

		while (stream_.read_buffer()->readableBytes() < total_len)
		{
			auto rd_buf = co_await stream_.read();
			if (!rd_buf)
			{
				LOG_ERROR
					<< "Connection closed by peer before reading body in fd "
					<< stream_.fd() << ". Method: " << method_name;
				throw std::runtime_error(
					"Server connection lost while reading body.");
			}
		}

		stream_.read_buffer()->retrieve(h.header_len);

		using RetTupleType = std::tuple<Ret>;
		RetTupleType ret_tuple;
		DeserializeTraits<RetTupleType>::deserialize(stream_.read_buffer(),
													 &ret_tuple);

		co_return std::get<0>(ret_tuple);
	}

  private:
	Stream stream_;
};
} // namespace rac

#endif