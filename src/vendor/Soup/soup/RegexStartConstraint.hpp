#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	template <bool escape_sequence, bool multi_line>
	struct RegexStartConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.begin)
			{
				return true;
			}
			if constexpr (multi_line)
			{
				if (*(m.it - 1) == '\n')
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			return escape_sequence ? "\\A" : "^";
		}

		void getFlags(uint16_t& set, uint16_t& unset) const noexcept final
		{
			if constexpr (!escape_sequence)
			{
				if constexpr (multi_line)
				{
					set |= RE_MULTILINE;
				}
				else
				{
					unset |= RE_MULTILINE;
				}
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
