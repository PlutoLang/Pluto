#pragma once

#include <cstdint>

namespace Pluto
{
	[[nodiscard]] constexpr uint32_t joaat(const char* data, size_t size)
	{
		/* do partial on input */
		size_t v3 = 0;
		uint32_t result = 0; /* initial = 0 */
		int v5 = 0;
		for (; v3 < size; result = ((uint32_t)(1025 * (v5 + result)) >> 6) ^ (1025 * (v5 + result))) {
			v5 = data[v3++];
		}

		/* finalise */
		result = (0x8001 * (((uint32_t)(9 * result) >> 11) ^ (9 * result)));

		return result;
	}

	[[nodiscard]] constexpr size_t constexpr_strlen(const char* str)
	{
		size_t len = 0;
		while (*str)
		{
			++len;
			++str;
		}
		return len;
	}

	[[nodiscard]] constexpr uint32_t joaat(const char* data)
	{
		return joaat(data, constexpr_strlen(data));
	}

	[[nodiscard]] inline uint32_t joaat(TString* ts)
	{
		return joaat(ts->contents, ts->size());
	}
}
