#pragma once

#include "Ipv6Maths.hpp"

NAMESPACE_SOUP
{
	struct CidrSubnet6
	{
		std::array<uint32_t, 4> addr;
		std::array<uint32_t, 4> mask;

		constexpr CidrSubnet6(const IpAddr& addr, uint8_t size) noexcept
			: addr(Ipv6Maths::fromIpAddr(addr)), mask(Ipv6Maths::generateBitmaskHi(size))
		{
			Ipv6Maths::bandEq(this->addr, mask);
		}

		[[nodiscard]] constexpr bool contains(const IpAddr& addr) const noexcept
		{
			return contains(Ipv6Maths::fromIpAddr(addr));
		}

		[[nodiscard]] constexpr bool contains(std::array<uint32_t, 4> addr) const noexcept
		{
			Ipv6Maths::bandEq(addr, this->mask);
			return Ipv6Maths::isEq(addr, this->addr);
		}

		[[nodiscard]] std::array<uint32_t, 4> random() const noexcept;

		[[nodiscard]] IpAddr getAddr() const noexcept
		{
			return Ipv6Maths::toIpAddr(addr);
		}

		[[nodiscard]] uint8_t getSize() const noexcept;
	};
}
