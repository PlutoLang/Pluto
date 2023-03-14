#pragma once

#include "base.hpp"

#include <array>
#include <cstdint>

namespace soup
{
	[[nodiscard]]
#if SOUP_CPP20
	consteval
#else
	inline
#endif
	std::array<uint8_t, 256> alpha2decodetbl(const char* tbl)
	{
		std::array<uint8_t, 256> out;
		out.fill(0xFF);
		for (uint8_t i = 0; tbl[i]; ++i)
		{
			out[tbl[i]] = i;
		}
		return out;
	}
}
