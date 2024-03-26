#pragma once

#include "RegexConstraint.hpp"

NAMESPACE_SOUP
{
	struct RegexDummyConstraint : public RegexConstraint
	{
		using RegexConstraint::RegexConstraint;

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 0;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			return {};
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}
	};
}
