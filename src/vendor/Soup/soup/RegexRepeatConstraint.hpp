#pragma once

#include "RegexConstraint.hpp"

#include "UniquePtr.hpp"

namespace soup
{
	template <bool at_least_one, bool greedy>
	struct RegexRepeatConstraint : public RegexConstraint
	{
		UniquePtr<RegexConstraint> constraint;

		RegexRepeatConstraint(UniquePtr<RegexConstraint>&& constraint)
			: constraint(std::move(constraint))
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			// Meta-constraint. Transitions will be set up to correctly handle matching of this.
			return true;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			std::string str = constraint->toString();
			if (at_least_one)
			{
				str.push_back('+');
			}
			else
			{
				str.push_back('*');
			}
			if (!greedy)
			{
				str.push_back('?');
			}
			return str;
		}
	};
}
