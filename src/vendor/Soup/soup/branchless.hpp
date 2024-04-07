#pragma once

#include <cstdint>
#include <type_traits>

#include "base.hpp"

#undef min
#undef max

NAMESPACE_SOUP
{
	struct branchless
	{
		template <typename T>
		[[nodiscard]] constexpr static std::enable_if_t<!std::is_pointer_v<T>, T> trinary(bool cond, T a, T b)
		{
			return (cond * a) + ((!cond) * b);
		}

		template <typename T>
		[[nodiscard]] constexpr static std::enable_if_t<std::is_pointer_v<T>, T> trinary(bool cond, T a, T b)
		{
			return reinterpret_cast<T>((cond * reinterpret_cast<uintptr_t>(a)) + ((!cond) * reinterpret_cast<uintptr_t>(b)));
		}

		template <typename T>
		[[nodiscard]] constexpr static T min(T a, T b)
		{
			return trinary(a < b, a, b);
		}

		template <typename T>
		[[nodiscard]] constexpr static T max(T a, T b)
		{
			return trinary(b < a, a, b);
		}
	};
}
