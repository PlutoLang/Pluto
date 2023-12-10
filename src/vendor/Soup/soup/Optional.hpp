#pragma once

#include <optional>

namespace soup
{
	template <typename T>
	struct Optional : public std::optional<T>
	{
		using Base = std::optional<T>;

		using Base::Base;

		[[nodiscard]] bool consume(T& outValue)
		{
			if (Base::has_value())
			{
				outValue = Base::value();
				return true;
			}
			return false;
		}
	};
}
