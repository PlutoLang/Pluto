#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	enum netStatus : uint8_t
	{
		NET_PENDING = 0,
		NET_OK,
		NET_FAIL_NO_DNS_RESPONSE,
		NET_FAIL_NO_DNS_RESULTS,
		NET_FAIL_L4_TIMEOUT,
		NET_FAIL_L4_ERROR,
		NET_FAIL_L7_TIMEOUT,
		NET_FAIL_L7_PREMATURE_END,
	};

	[[nodiscard]] const char* netStatusToString(netStatus status);
}
