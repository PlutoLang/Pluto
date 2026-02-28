#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	template <bool unicode>
	struct RegexPositiveLookbehindConstraint : public RegexConstraintLookbehindImpl<unicode>
	{
		using Base = RegexConstraintLookbehindImpl<unicode>;

		using Base::Base;

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			uint16_t set = 0;
			uint16_t unset = 0;
			if constexpr (unicode)
			{
				set |= RE_UNICODE;
			}
			else
			{
				unset |= RE_UNICODE;
			}
			RegexConstraint::updateFlags(str, flags, set, unset);
			str.append("(?<=");
			Base::group.toString(str, flags);
			str.push_back(')');
		}
	};
}
