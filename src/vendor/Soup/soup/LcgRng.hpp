#pragma once

#include <cstdint>

#include "Endian.hpp"

NAMESPACE_SOUP
{
	// Linear congruential generator (LCG)
	struct LcgRng
	{
		uint64_t state;
		// modulus 2^64 (aka. overflowing a 64-bit integer)
		uint64_t multiplier = 6364136223846793005ull;
		uint64_t increment = 1442695040888963407ull;

		LcgRng();

		constexpr LcgRng(uint64_t seed)
			: state(seed)
		{
		}

		constexpr void skip() noexcept
		{
			state *= multiplier;
			state += increment;
		}

		constexpr uint64_t generate() noexcept
		{
			skip();
			return Endianness::invert(state); // invert byte order since the higher-order bits have longer periods
		}

		constexpr uint8_t generateByte() noexcept
		{
			skip();
			return state >> (8 * 7); // use highest byte for better entropy
		}
	};
}
