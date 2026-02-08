#pragma once

#include "base.hpp"

#if SOUP_CPP20
#include <bit>
#endif

NAMESPACE_SOUP
{
#if !SOUP_CPP20
	template <typename T>
	[[nodiscard]] constexpr T rotr(const T val, int shift) noexcept;
#endif

	template <typename T>
	[[nodiscard]] constexpr T rotl(const T val, int shift) noexcept
	{
#if SOUP_CPP20
		return std::rotl<T>(val, shift);
#else
		const auto bits = sizeof(T) * 8;
		shift %= bits;
		if (shift > 0)
		{
			return (val << shift) | (val >> (bits - shift));
		}
		else if (shift == 0)
		{
			return val;
		}
		else // if (shift < 0)
		{
			return rotr(val, -shift);
		}
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr T rotr(const T val, int shift) noexcept
	{
#if SOUP_CPP20
		return std::rotr<T>(val, shift);
#else
		const auto bits = sizeof(T) * 8;
		shift %= bits;
		if (shift > 0)
		{
			return (val >> shift) | (val << (bits - shift));
		}
		else if (shift == 0)
		{
			return val;
		}
		else // if (shift < 0)
		{
			return rotl(val, -shift);
		}
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr int popcount(const T val) noexcept
	{
#if SOUP_CPP20
		return std::popcount<T>(val);
#else
		if constexpr (sizeof(T) <= 4)
		{
			return __builtin_popcount(val);
		}
		return __builtin_popcountll(val);
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr int countl_zero(const T val) noexcept
	{
#if SOUP_CPP20
		return std::countl_zero<T>(val);
#else
		if (val)
		{
			if constexpr (sizeof(T) <= 4)
			{
				return __builtin_clz(val);
			}
			return __builtin_clzll(val);
		}
		return sizeof(T) * 8;
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr int countr_zero(const T val) noexcept
	{
#if SOUP_CPP20
		return std::countr_zero<T>(val);
#else
		if (val)
		{
			if constexpr (sizeof(T) <= 4)
			{
				return __builtin_ctz(val);
			}
			return __builtin_ctzll(val);
		}
		return sizeof(T) * 8;
#endif
	}
}
