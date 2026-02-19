#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#define M_TAU (M_PI * 2.0)

#define DEG_TO_RAD(deg) ((deg) * (float(M_PI) / 180.0f))
#define DEG_TO_HALF_RAD(deg) ((deg) * (float(M_PI) / 360.0f))

#define RAD_TO_DEG(rad) ((rad) / (float(M_PI) / 180.0f))

#include "base.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	template <typename T = float>
	[[nodiscard]] constexpr T lerp(T a, T b, float t) noexcept
	{
		return (T)(a + (b - a) * t);
	}

	template <typename T>
	[[nodiscard]] constexpr T pow(T x, T p) noexcept // p must be >= 0
	{
		// Stolen from https://stackoverflow.com/a/1505791
		// Could be better: https://stackoverflow.com/a/101613 (also see comments)
		// Tho this should still work for ints and floats.
		if (p == T(0))
		{
			return 1;
		}
		if (p == T(1))
		{
			return x;
		}
		auto tmp = pow(x, p / T(2));
		if (p % T(2) == T(0))
		{
			return tmp * tmp;
		}
		return x * tmp * tmp;
	}

	template <typename T, SOUP_RESTRICT(std::is_unsigned_v<T>)>
	[[nodiscard]] constexpr T can_add_without_overflow(T a, T b) noexcept
	{
		return a + b >= a;
	}
}
