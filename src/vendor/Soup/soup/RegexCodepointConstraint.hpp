#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	struct RegexCodepointConstraint : public RegexConstraint
	{
		std::string c;

		RegexCodepointConstraint(std::string c)
			: c(std::move(c))
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (static_cast<size_t>(std::distance(m.it, m.end)) < c.size())
			{
				return false;
			}
			for (size_t i = 0; i != c.size(); ++i)
			{
				if (m.it[i] != c[i])
				{
					return false;
				}
			}
			m.it += c.size();
			return true;
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			updateFlags(str, flags, RE_UNICODE, 0);
			str.append(c);
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto cc = soup::make_unique<RegexCodepointConstraint>(c);
			success_transitions.setTransitionTo(cc->getEntrypoint());
			success_transitions.emplace(&cc->success_transition);
			return cc;
		}
	};
}
