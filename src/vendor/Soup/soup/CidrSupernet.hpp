#pragma once

#include <array>

#include "rand.hpp"

NAMESPACE_SOUP
{
	template <typename Subnet, size_t Size>
	struct CidrSupernet
	{
		std::array<Subnet, Size> subnets;

		constexpr CidrSupernet(std::array<Subnet, Size>&& subnets)
			: subnets(std::move(subnets))
		{
		}

		template <typename T = IpAddr>
		[[nodiscard]] constexpr bool contains(const T& addr) const noexcept
		{
			for (const auto& subnet : subnets)
			{
				if (subnet.contains(addr))
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] auto random() const noexcept
		{
			if constexpr (Size == 1)
			{
				return subnets.at(0).random();
			}
			return subnets.at(rand(0, subnets.size() - 1)).random();
		}
	};
}
