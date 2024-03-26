#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"
#include "string.hpp"

namespace soup
{
	template <bool inverted>
	struct RegexWordCharConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return string::isWordChar(*m.it++) ^ inverted;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			return inverted ? "\\W" : "\\w";
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}
	};
}
