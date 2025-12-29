#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct CompiletimePatternBase
	{
		static constexpr size_t countBytes(const char* sig)
		{
			size_t count = 1;
			for (auto i = 0; sig[i]; i++)
			{
				if (sig[i] == ' ')
				{
					count++;
				}
			}

			return count;
		}

		static constexpr int32_t hex_to_int(const char c)
		{
			if (c >= 'a' && c <= 'f')
			{
				return static_cast<int32_t>(c) - 87;
			}
			if (c >= 'A' && c <= 'F')
			{
				return static_cast<int32_t>(c) - 55;
			}
			if (c >= '0' && c <= '9')
			{
				return static_cast<int32_t>(c) - 48;
			}
			return {};
		}
	};
}
