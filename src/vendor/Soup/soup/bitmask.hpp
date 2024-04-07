#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct bitmask
	{
		template <typename T>
		[[nodiscard]] static constexpr T generateLo(uint8_t width) noexcept
		{
			constexpr auto T_width = (sizeof(T) * 8);
			if (width == T_width)
			{
				T bitmask = 0;
				for (uint8_t i = 0; i != width; ++i)
				{
					bitmask |= (((T)1) << i);
				}
				return bitmask;
			}
			return (((T)1) << width) - 1;
		}

		template <typename T>
		[[nodiscard]] static constexpr T generateHi(uint8_t width) noexcept
		{
			if (width == 0)
			{
				return 0;
			}
			constexpr auto T_width = (sizeof(T) * 8);
			if (width == T_width)
			{
				T bitmask = 0;
				for (uint8_t i = T_width - 1; width != 0; --i, --width)
				{
					bitmask |= (((T)1) << i);
				}
				return bitmask;
			}
			return ((((T)1) << width) - 1) << (T_width - width);
		}
	};

	static_assert(bitmask::generateLo<uint32_t>(0) == 0);
	static_assert(bitmask::generateLo<uint32_t>(22) == 0b1111111111111111111111);
	static_assert(bitmask::generateLo<uint64_t>(22) == 0b1111111111111111111111);

	static_assert(bitmask::generateHi<uint32_t>(0) == 0);
	static_assert(bitmask::generateHi<uint32_t>(22) == 0b11111111111111111111110000000000);
}
