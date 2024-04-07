#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum dnsClass : uint16_t
	{
		DNS_IN = 1,
		DNS_CS = 2,
		DNS_CH = 3,
		DNS_HS = 4,
	};
}
