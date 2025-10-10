#pragma once

#include <cstdint>
#include <cstring> // memcpy
#include <string>

#include "base.hpp"
#include "fwd.hpp"

#if SOUP_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "Endian.hpp"
#include "stringifyable.hpp"

#define SOUP_IPV4(a, b, c, d) ::soup::native_u32_t((a << 24) | (b << 16) | (c << 8) | d)
#define SOUP_IPV4_NWE(a, b, c, d) ((soup::ENDIAN_NATIVE == soup::ENDIAN_NETWORK) ? ::soup::network_u32_t((a << 24) | (b << 16) | (c << 8) | d) : ::soup::network_u32_t((d << 24) | (c << 16) | (b << 8) | a))

NAMESPACE_SOUP
{
	class IpAddr
	{
	public:
		union
		{
			in6_addr data;
			uint16_t shorts[8]; // <- constexpr
			uint8_t bytes[16];
		};

		[[nodiscard]] SOUP_PURE uint64_t* /*[2]*/ longs() noexcept { return reinterpret_cast<uint64_t*>(this); }
		[[nodiscard]] SOUP_PURE uint32_t* /*[4]*/ ints() noexcept { return reinterpret_cast<uint32_t*>(this); }

		constexpr IpAddr() noexcept
			: shorts{ 0, 0, 0, 0, 0, 0, 0, 0 }
		{
		}

		constexpr IpAddr(const IpAddr& b) noexcept
			: IpAddr(b.shorts)
		{
		}

		explicit IpAddr(const uint32_t* ints) noexcept
		{
			for (auto i = 0; i != 4; ++i)
			{
				this->ints()[i] = ints[i];
			}
		}

		explicit constexpr IpAddr(const uint16_t* shorts) noexcept
			: shorts{ shorts[0], shorts[1], shorts[2], shorts[3], shorts[4], shorts[5], shorts[6], shorts[7] }
		{
		}

		explicit IpAddr(const uint8_t* bytes) noexcept
		{
			for (auto i = 0; i != 16; ++i)
			{
				this->bytes[i] = bytes[i];
			}
		}

		explicit constexpr IpAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k, uint8_t l, uint8_t m, uint8_t n, uint8_t o, uint8_t p)
			: IpAddr(
				native_u16_t(((uint16_t)a << 8) | b),
				native_u16_t(((uint16_t)c << 8) | d),
				native_u16_t(((uint16_t)e << 8) | f),
				native_u16_t(((uint16_t)g << 8) | h),
				native_u16_t(((uint16_t)i << 8) | j),
				native_u16_t(((uint16_t)k << 8) | l),
				native_u16_t(((uint16_t)m << 8) | n),
				native_u16_t(((uint16_t)o << 8) | p)
			)
		{
		}

		explicit constexpr IpAddr(native_u16_t a, native_u16_t b, native_u16_t c, native_u16_t d, native_u16_t e, native_u16_t f, native_u16_t g, native_u16_t h)
			: shorts{
				Endianness::toNetwork(a).data,
				Endianness::toNetwork(b).data,
				Endianness::toNetwork(c).data,
				Endianness::toNetwork(d).data,
				Endianness::toNetwork(e).data,
				Endianness::toNetwork(f).data,
				Endianness::toNetwork(g).data,
				Endianness::toNetwork(h).data
			}
		{
		}

		// THIS IS FOR IPv6! USE `SOUP_IPV4` macro for easy IPv4 construction.
		explicit constexpr IpAddr(native_u32_t a, native_u32_t b, native_u32_t c, native_u32_t d)
			: IpAddr(
				native_u16_t(a), native_u16_t(a >> 16),
				native_u16_t(b), native_u16_t(b >> 16),
				native_u16_t(c), native_u16_t(c >> 16),
				native_u16_t(d), native_u16_t(d >> 16)
			)
		{
		}

		explicit constexpr IpAddr(native_u64_t hi, native_u64_t lo)
			: IpAddr(
				native_u32_t((uint32_t)hi), native_u32_t(hi >> 32),
				native_u32_t((uint32_t)lo), native_u32_t(lo >> 32)
			)
		{
		}

		SOUP_CONSTEXPR20 IpAddr(const network_u32_t ipv4) noexcept
		{
			operator =(ipv4);
		}

		SOUP_CONSTEXPR20 IpAddr(const native_u32_t ipv4) noexcept
		{
			operator =(ipv4);
		}

		bool fromString(const char* str) SOUP_EXCAL;
		bool fromString(const std::string& str) SOUP_EXCAL;

		constexpr void operator = (const network_u32_t ipv4) noexcept
		{
			maskToV4();
			// *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(&data) + 12) = ipv4;
			shorts[6] = ipv4;
			shorts[7] = ipv4 >> 16;
		}

		constexpr void operator = (const native_u32_t ipv4) noexcept
		{
			operator = (Endianness::toNetwork(ipv4));
		}

		[[nodiscard]] bool operator ==(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) == 0;
		}

		[[nodiscard]] bool operator !=(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) != 0;
		}

		[[nodiscard]] bool operator <(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) < 0;
		}

		[[nodiscard]] bool operator <=(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) <= 0;
		}

		[[nodiscard]] bool operator >(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) > 0;
		}

		[[nodiscard]] bool operator >=(const IpAddr& b) const noexcept
		{
			return memcmp(reinterpret_cast<const void*>(&data), reinterpret_cast<const void*>(&b.data), sizeof(data)) >= 0;
		}

		void reset() noexcept
		{
			memset(&data, 0, sizeof(data));
		}

		// Checks for [::] and 0.0.0.0
		[[nodiscard]] bool isZero() const noexcept
		{
			if (isV4())
			{
				return getV4() == 0;
			}
			for (const auto& s : shorts)
			{
				if (s != 0)
				{
					return false;
				}
			}
			return true;
		}

		[[nodiscard]] bool isLoopback() const noexcept;
		[[nodiscard]] bool isPrivate() const noexcept;
		[[nodiscard]] bool isLocalnet() const noexcept; // Loopback or private network?

		[[nodiscard]] bool isV4() const noexcept
		{
			return IN6_IS_ADDR_V4MAPPED(&data);
		}

		[[nodiscard]] network_u32_t getV4() const noexcept
		{
			return *reinterpret_cast<const network_u32_t*>(reinterpret_cast<uintptr_t>(&data) + 12);
		}

		[[nodiscard]] native_u32_t getV4NativeEndian() const noexcept
		{
			return Endianness::toNative(getV4());
		}

	private:
		constexpr void maskToV4() noexcept
		{
			shorts[0] = 0;
			shorts[1] = 0;
			shorts[2] = 0;
			shorts[3] = 0;
			shorts[4] = 0;
			shorts[5] = 0xffff;
		}

	public:
		SOUP_STRINGIFYABLE(IpAddr)

		[[nodiscard]] std::string toString() const noexcept
		{
			if (isV4())
			{
				return toString4();
			}
			return toString6();
		}

		[[nodiscard]] std::string toStringForAddr() const noexcept
		{
			if (isV4())
			{
				return toString4();
			}
			std::string str(1, '[');
			str.append(toString6());
			str.push_back(']');
			return str;
		}

		[[nodiscard]] std::string toString4() const noexcept
		{
			char buf[INET_ADDRSTRLEN] = { '\0' };
			inet_ntop(AF_INET, reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(&data) + 12), buf, INET_ADDRSTRLEN);
			return buf;
		}

		[[nodiscard]] std::string toString6() const noexcept
		{
			char buf[INET6_ADDRSTRLEN] = { '\0' };
			inet_ntop(AF_INET6, &data, buf, INET6_ADDRSTRLEN);
			return buf;
		}

		[[nodiscard]] std::string getArpaName() const;
#if !SOUP_WASM
		[[nodiscard]] std::string getReverseDns() const;
		[[nodiscard]] std::string getReverseDns(dnsResolver& resolver) const;
#endif
	};
	static_assert(sizeof(IpAddr) == 16);
}
