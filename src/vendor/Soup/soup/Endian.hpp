#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#include <cstdlib> // _byteswap_*
#include <type_traits> // is_constant_evaluated
#endif

#include "base.hpp"

#include "IntStruct.hpp"

NAMESPACE_SOUP
{
	enum Endian : bool
	{
		ENDIAN_LITTLE = true,
		ENDIAN_BIG = false,
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		ENDIAN_NATIVE = ENDIAN_LITTLE,
#else
		ENDIAN_NATIVE = ENDIAN_BIG,
#endif
		ENDIAN_NETWORK = ENDIAN_BIG,
	};

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ // Native endian is not little endian?
	static_assert(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__); // Then it should be big endian.
#endif

	SOUP_INT_STRUCT(native_u16_t, uint16_t);
	SOUP_INT_STRUCT(native_u32_t, uint32_t);
	SOUP_INT_STRUCT(native_u64_t, uint64_t);
	SOUP_INT_STRUCT(network_u16_t, uint16_t);
	SOUP_INT_STRUCT(network_u32_t, uint32_t);
	SOUP_INT_STRUCT(network_u64_t, uint64_t);

	struct Endianness
	{
		[[nodiscard]] static constexpr uint16_t invert(uint16_t val) noexcept
		{
#if SOUP_CPP23
			return std::byteswap<uint16_t>(val);
#elif defined(_MSC_VER) && !defined(__clang__)
	#if SOUP_CPP20
			if (!std::is_constant_evaluated())
			{
				return _byteswap_ushort(val);
			}
	#endif
#elif defined(__GNUC__) || defined(__clang__)
			return __builtin_bswap16(val);
#endif
			return (val << 8) | (val >> 8);
		}
		
		[[nodiscard]] static constexpr uint32_t invert(uint32_t val) noexcept
		{
#if SOUP_CPP23
			return std::byteswap<uint32_t>(val);
#elif defined(_MSC_VER) && !defined(__clang__)
	#if SOUP_CPP20
			if (!std::is_constant_evaluated())
			{
				return _byteswap_ulong(val);
			}
	#endif
#elif defined(__GNUC__) || defined(__clang__)
			return __builtin_bswap32(val);
#endif
			return val << (32 - 8)
				| ((val >> 8) & 0xFF) << (32 - 16)
				| ((val >> 16) & 0xFF) << (32 - 24)
				| ((val >> 24) & 0xFF)
				;
		}

		[[nodiscard]] static constexpr uint64_t invert(uint64_t val) noexcept
		{
#if SOUP_CPP23
			return std::byteswap<uint64_t>(val);
#elif defined(_MSC_VER) && !defined(__clang__)
	#if SOUP_CPP20
			if (!std::is_constant_evaluated())
			{
				return _byteswap_uint64(val);
			}	
	#endif
#elif defined(__GNUC__) || defined(__clang__)
			return __builtin_bswap64(val);
#endif
			return val << (64 - 8)
				| ((val >> 8) & 0xFF) << (64 - 16)
				| ((val >> 16) & 0xFF) << (64 - 24)
				| ((val >> 24) & 0xFF) << (64 - 32)
				| ((val >> 32) & 0xFF) << (64 - 40)
				| ((val >> 40) & 0xFF) << (64 - 48)
				| ((val >> 48) & 0xFF) << (64 - 56)
				| ((val >> 56) & 0xFF)
				;
		}

		[[nodiscard]] static constexpr network_u16_t toNetwork(uint16_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val);
			}
			return network_u16_t(val);
		}

		[[nodiscard]] static constexpr network_u32_t toNetwork(uint32_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val);
			}
			return network_u32_t(val);
		}

		[[nodiscard]] static constexpr network_u64_t toNetwork(uint64_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val);
			}
			return network_u64_t(val);
		}

		[[nodiscard]] static constexpr native_u16_t toNative(network_u16_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val.data);
			}
			return native_u16_t(val.data);
		}

		[[nodiscard]] static constexpr native_u32_t toNative(network_u32_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val.data);
			}
			return native_u32_t(val.data);
		}

		[[nodiscard]] static constexpr native_u64_t toNative(network_u64_t val) noexcept
		{
			if constexpr (ENDIAN_NATIVE != ENDIAN_NETWORK)
			{
				return invert(val.data);
			}
			return native_u64_t(val.data);
		}
	};
}
