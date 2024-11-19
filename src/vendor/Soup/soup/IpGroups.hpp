#pragma once

#include "CidrSubnet4.hpp"
#include "CidrSubnet6.hpp"
#include "CidrSupernet.hpp"

NAMESPACE_SOUP
{
	struct IpGroups
	{
		// IPv4

		static constexpr auto v4_loopback = CidrSubnet4(SOUP_IPV4(127, 0, 0, 0), 8);

		static constexpr CidrSupernet v4_private_network = {
			std::array {
				CidrSubnet4(SOUP_IPV4(192, 168, 0, 0), 16),
				CidrSubnet4(SOUP_IPV4(172, 16, 0, 0), 12),
				CidrSubnet4(SOUP_IPV4(10, 0, 0, 0), 8),
			}
		};

		static constexpr CidrSupernet v4_misc_reserved = {
			std::array {
				CidrSubnet4(SOUP_IPV4(0, 0, 0, 0), 8),
				CidrSubnet4(SOUP_IPV4(255, 255, 255, 255), 32),
			}
		};

		// IPv6

		static constexpr auto v6_loopback = CidrSubnet6(IpAddr(0, 0, 0, 0, 0, 0, 0, 1), 128);
	};
}
