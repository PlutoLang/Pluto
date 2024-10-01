#pragma once

#include "RegexConstraint.hpp"

NAMESPACE_SOUP
{
	template <bool escape_sequence, bool multi_line, bool end_only>
	struct RegexEndConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.end)
			{
				return true;
			}
			if constexpr (multi_line)
			{
				if (*m.it == '\n')
				{
					return true;
				}
			}
			else if constexpr (!end_only)
			{
				if ((m.it + 1) == m.end
					&& *m.it == '\n'
					)
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			if constexpr (escape_sequence)
			{
				static_assert(multi_line == false);
				return end_only ? "\\z" : "\\Z";
			}
			return "$";
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
				if constexpr (end_only)
				{
					set |= RE_DOLLAR_ENDONLY;
				}
				else
				{
					unset |= RE_DOLLAR_ENDONLY;
				}
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
