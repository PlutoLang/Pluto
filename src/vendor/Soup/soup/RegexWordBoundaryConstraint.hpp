#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	template <bool inverted>
	struct RegexWordBoundaryConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.begin
				|| m.it == m.end
				)
			{
				return true ^ inverted;
			}
			if (string::isWordChar(*(m.it - 1)))
			{
				return !string::isWordChar(*m.it) ^ inverted;
			}
			else
			{
				return string::isWordChar(*m.it) ^ inverted;
			}
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			if constexpr (inverted)
			{
				str.append("\\B");
			}
			else
			{
				str.append("\\b");
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
