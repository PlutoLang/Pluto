#pragma once

#include <algorithm>
#include <array>

#include "bitmask.hpp"
#include "bitutil.hpp"
#include "IpAddr.hpp"

#undef min
#undef max

NAMESPACE_SOUP
{
	struct Ipv6Maths
	{
		[[nodiscard]] static constexpr std::array<uint32_t, 4> generateBitmaskHi(int16_t width) noexcept // 0 <= width <= 128
		{
			return {
				bitmask::generateHi<uint32_t>(static_cast<uint8_t>(std::min<int16_t>(width, 32))),
				bitmask::generateHi<uint32_t>(static_cast<uint8_t>(std::clamp<int16_t>(width - 32, 0, 32))),
				bitmask::generateHi<uint32_t>(static_cast<uint8_t>(std::clamp<int16_t>(width - 64, 0, 32))),
				bitmask::generateHi<uint32_t>(static_cast<uint8_t>(std::max<int16_t>(width - 96, 0))),
			};
		}

		[[nodiscard]] static IpAddr toIpAddr(const std::array<uint32_t, 4>& arr) noexcept
		{
			IpAddr addr;
			addr.ints()[0] = Endianness::toNetwork(arr.at(0));
			addr.ints()[1] = Endianness::toNetwork(arr.at(1));
			addr.ints()[2] = Endianness::toNetwork(arr.at(2));
			addr.ints()[3] = Endianness::toNetwork(arr.at(3));
			return addr;
		}

		[[nodiscard]] static constexpr std::array<uint32_t, 4> fromIpAddr(const IpAddr& ip) noexcept
		{
			return {
				// Concise code is not supported in a constant expression
				//Endianness::toNative(network_u32_t(ip.ints[0])),
				//Endianness::toNative(network_u32_t(ip.ints[1])),
				//Endianness::toNative(network_u32_t(ip.ints[2])),
				//Endianness::toNative(network_u32_t(ip.ints[3])),
				((uint32_t)Endianness::toNative(network_u16_t(ip.shorts[0])).data << 16) | Endianness::toNative(network_u16_t(ip.shorts[1])).data,
				((uint32_t)Endianness::toNative(network_u16_t(ip.shorts[2])).data << 16) | Endianness::toNative(network_u16_t(ip.shorts[3])).data,
				((uint32_t)Endianness::toNative(network_u16_t(ip.shorts[4])).data << 16) | Endianness::toNative(network_u16_t(ip.shorts[5])).data,
				((uint32_t)Endianness::toNative(network_u16_t(ip.shorts[6])).data << 16) | Endianness::toNative(network_u16_t(ip.shorts[7])).data,
			};
		}

		[[nodiscard]] static constexpr bool isEq(const std::array<uint32_t, 4>& a, const std::array<uint32_t, 4>& b) noexcept
		{
			for (auto i = 0; i != 4; ++i)
			{
				if (a[i] != b[i])
				{
					return false;
				}
			}
			return true;
		}

		static constexpr void bandEq(std::array<uint32_t, 4>& l, const std::array<uint32_t, 4>& r) noexcept
		{
			for (auto i = 0; i != 4; ++i)
			{
				l[i] &= r[i];
			}
		}

		static constexpr void xorEq(std::array<uint32_t, 4>& l, const std::array<uint32_t, 4>& r) noexcept
		{
			for (auto i = 0; i != 4; ++i)
			{
				l[i] ^= r[i];
			}
		}

		[[nodiscard]] static constexpr int16_t getMostSignificantSetBit(const std::array<uint32_t, 4>& a) noexcept
		{
			int16_t offset = 96;
			for (auto i = 0; i != 4; ++i, offset -= 32)
			{
				if (a[i] != 0)
				{
					return offset + bitutil::getMostSignificantSetBit(a[i]);
				}
			}
			return -1;
		}
	};
}
