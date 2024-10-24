#pragma once

#include "RegexConstraintLookbehind.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	template <bool unicode>
	struct RegexNegativeLookbehindConstraint : public RegexConstraintLookbehindImpl<unicode>
	{
		using Base = RegexConstraintLookbehindImpl<unicode>;

		using Base::Base;

		[[nodiscard]] std::string toString() const noexcept final
		{
			auto str = Base::group.toString();
			str.insert(0, "(?<!");
			str.push_back(')');
			return str;
		}
	};
}
