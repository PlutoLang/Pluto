#pragma once

#include "RegexConstraint.hpp"

#include <vector>

#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct RegexOpenEndedRangeQuantifierConstraintBase : public RegexConstraint
	{
		std::vector<UniquePtr<RegexConstraint>> constraints;

		[[nodiscard]] RegexConstraint* getEntrypoint() noexcept final
		{
			return constraints.at(0)->getEntrypoint();
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			// Meta-constraint. Transitions will be set up to correctly handle matching of this.
			return true;
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return constraints.at(0)->getCursorAdvancement() * constraints.size();
		}
	};

	template <bool greedy>
	struct RegexOpenEndedRangeQuantifierConstraint : public RegexOpenEndedRangeQuantifierConstraintBase
	{
		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			constraints.at(0)->toString(str, flags);
			str.push_back('{');
			str.append(std::to_string(constraints.size()));
			str.append(",}");
			if (!greedy)
			{
				str.push_back('?');
			}
		}
	};
}
