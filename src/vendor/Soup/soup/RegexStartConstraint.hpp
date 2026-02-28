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

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			if constexpr (escape_sequence)
			{
				str.append("\\A");
			}
			else
			{
				uint16_t set = 0;
				uint16_t unset = 0;
				if constexpr (multi_line)
				{
					set |= RE_MULTILINE;
				}
				else
				{
					unset |= RE_MULTILINE;
				}
				updateFlags(str, flags, set, unset);
				str.push_back('^');
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
