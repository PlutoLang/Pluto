#pragma once

#include "RegexConstraint.hpp"

#include "RegexGroup.hpp"
#include "RegexMatcher.hpp"

namespace soup
{
	struct RegexNegativeLookaheadConstraint : public RegexConstraint
	{
		RegexGroup group;

		RegexNegativeLookaheadConstraint(const RegexGroup::ConstructorState& s)
			: group(s, true)
		{
		}

		[[nodiscard]] const RegexConstraint* getEntrypoint() const noexcept final
		{
			return group.initial;
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			m.restoreCheckpoint();
			return true;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			auto str = group.toString();
			str.insert(0, "(?!");
			str.push_back(')');
			return str;
		}

		void getFlags(uint16_t& set, uint16_t& unset) const noexcept final
		{
			group.getFlags(set, unset);
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}
	};
}
