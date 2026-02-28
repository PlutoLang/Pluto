#pragma once

#include "RegexConstraint.hpp"

#include "RegexGroup.hpp"
#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	struct RegexPositiveLookaheadConstraint : public RegexConstraint
	{
		RegexGroup group;

		RegexPositiveLookaheadConstraint(const RegexGroup::ConstructorState& s)
			: group(s, true)
		{
		}

		[[nodiscard]] RegexConstraint* getEntrypoint() noexcept final
		{
			return group.initial;
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			m.restoreCheckpoint();
			return true;
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			str.append("(?=");
			group.toString(str, flags);
			str.push_back(')');
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
