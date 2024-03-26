#pragma once

#include "RegexConstraint.hpp"

#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct RegexOptConstraint : public RegexConstraint
	{
		UniquePtr<RegexConstraint> constraint;

		RegexOptConstraint(UniquePtr<RegexConstraint>&& constraint)
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
			str.push_back('?');
			return str;
		}
	};
}
