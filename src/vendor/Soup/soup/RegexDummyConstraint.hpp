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

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}
	};
}
