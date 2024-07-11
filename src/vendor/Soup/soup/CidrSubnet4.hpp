#pragma once

#include "bitmask.hpp"
#include "bitutil.hpp"
#include "IpAddr.hpp"
#include "rand.hpp"

NAMESPACE_SOUP
{
	struct CidrSubnet4
	{
		native_u32_t addr;
		uint32_t mask;

		constexpr CidrSubnet4(native_u32_t addr, uint8_t size) noexcept
			: addr(addr.data & bitmask::generateHi<uint32_t>(size)), mask(bitmask::generateHi<uint32_t>(size))
		{
		}

		constexpr CidrSubnet4(network_u32_t addr, uint8_t size) noexcept
			: CidrSubnet4(Endianness::toNative(addr), size)
		{
		}

		CidrSubnet4(const IpAddr& addr, uint8_t size) noexcept
			: CidrSubnet4(addr.getV4NativeEndian(), size)
		{
		}

		[[nodiscard]] SOUP_CONSTEXPR20 bool contains(const IpAddr& addr) const noexcept
		{
			return contains(addr.getV4NativeEndian());
		}

		[[nodiscard]] constexpr bool contains(const native_u32_t addr) const noexcept
		{
			return this->addr.data == (addr.data & this->mask);
		}

		[[nodiscard]] native_u32_t random() const noexcept
		{
			return addr.data | rand.t<uint32_t>(0, ~mask);
		}

		[[nodiscard]] IpAddr getAddr() const noexcept
		{
			return IpAddr(addr);
		}

		[[nodiscard]] uint8_t getSize() const noexcept
		{
			if (mask == 0)
			{
				return 0;
			}
			return static_cast<uint8_t>(32 - bitutil::getLeastSignificantSetBit(mask));
		}

		[[nodiscard]] native_u32_t getLower() const noexcept
		{
			return addr;
		}

		[[nodiscard]] native_u32_t getUpper() const noexcept
		{
			return addr | ~mask;
		}
	};
}
