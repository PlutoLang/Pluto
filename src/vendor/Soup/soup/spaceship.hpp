#pragma once

#include "base.hpp"

#define SOUP_SPACESHIP_USE_STD SOUP_CPP20

#if SOUP_SPACESHIP_USE_STD
#include <compare>
#endif

#if SOUP_SPACESHIP_USE_STD
NAMESPACE_SOUP
{
	using strong_ordering = ::std::strong_ordering;
}

#define SOUP_SPACESHIP(a, b) ((a) <=> (b))

[[nodiscard]] inline int SOUP_STRONG_ORDERING_TO_INT(soup::strong_ordering so)
{
	if (so != 0)
	{
		return so < 0 ? -1 : 1;
	}
	return 0;
}
#else
NAMESPACE_SOUP
{
	class strong_ordering
	{
	private:
		int value;

	public:
		static const strong_ordering less;
		static const strong_ordering equal;
		static const strong_ordering greater;

		constexpr strong_ordering(int value) noexcept
			: value(value)
		{
		}

		constexpr operator int() const noexcept
		{
			return value;
		}
	};
}

template <typename T>
[[nodiscard]] inline soup::strong_ordering SOUP_SPACESHIP(const T& a, const T& b)
{
	if (a == b)
	{
		return soup::strong_ordering::equal;
	}
	if (a < b)
	{
		return soup::strong_ordering::less;
	}
	return soup::strong_ordering::greater;
}

#define SOUP_STRONG_ORDERING_TO_INT(so) ((so).operator int())
#endif
