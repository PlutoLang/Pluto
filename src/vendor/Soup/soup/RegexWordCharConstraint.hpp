#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	template <bool inverted>
	struct RegexWordCharConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return string::isWordChar(*m.it++) ^ inverted;
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			if constexpr (inverted)
			{
				str.append("\\W");
			}
			else
			{
				str.append("\\w");
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto cc = soup::make_unique<RegexWordCharConstraint>();
			success_transitions.setTransitionTo(cc->getEntrypoint());
			success_transitions.emplace(&cc->success_transition);
			return cc;
		}
	};
}
