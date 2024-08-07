#pragma once

#include "RegexConstraint.hpp"

NAMESPACE_SOUP
{
	struct RegexConstraintLookbehind : public RegexConstraint
	{
		RegexGroup group;
		size_t window;

		RegexConstraintLookbehind(const RegexGroup::ConstructorState& s)
			: group(s, true)
		{
		}

		[[nodiscard]] RegexConstraint* getEntrypoint() noexcept final
		{
			return group.initial;
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};

	template <bool unicode>
	struct RegexConstraintLookbehindImpl : public RegexConstraintLookbehind
	{
		using RegexConstraintLookbehind::RegexConstraintLookbehind;

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if constexpr (unicode)
			{
				for (size_t i = 0; i != window; ++i)
				{
					if (m.begin == m.it)
					{
						return false;
					}
					if (UTF8_IS_CONTINUATION(*m.it))
					{
						return false;
					}
					unicode::utf8_sub(m.it, m.begin);
				}
			}
			else
			{
				if (static_cast<size_t>(std::distance(m.begin, m.it)) < window)
				{
					return false;
				}
				m.it -= window;
			}
			return true;
		}

		void getFlags(uint16_t& set, uint16_t& unset) const noexcept final
		{
			group.getFlags(set, unset);
			if constexpr (unicode)
			{
				set |= RE_UNICODE;
			}
		}
	};
}
