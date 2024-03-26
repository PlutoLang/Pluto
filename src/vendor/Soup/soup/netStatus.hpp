#pragma once

#include <cstdint>

namespace soup
{
	enum netStatus : uint8_t
	{
		NET_PENDING = 0,
		NET_OK,
		NET_FAIL_NO_DNS_RESULTS,
		NET_FAIL_L4_TIMEOUT,
		NET_FAIL_L4_ERROR,
		NET_FAIL_L7_TIMEOUT,
	};

	[[nodiscard]] const char* netStatusToString(netStatus status);
}
