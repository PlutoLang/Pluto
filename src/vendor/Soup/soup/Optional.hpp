#pragma once

#include <optional>

#include "base.hpp"

NAMESPACE_SOUP
{
	template <typename T>
	struct Optional : public std::optional<T>
	{
		using Base = std::optional<T>;

		using Base::Base;

		bool consume(T& outValue)
		{
			if (Base::has_value())
			{
				outValue = std::move(Base::value());
				return true;
			}
			return false;
		}
	};
}
