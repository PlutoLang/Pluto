#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	struct RegexCharConstraint : public RegexConstraint
	{
		char c;

		RegexCharConstraint(char c)
			: c(c)
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.end)
			{
				return false;
			}
			if (*m.it != c)
			{
				return false;
			}
			++m.it;
			return true;
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			switch (c)
			{
			case '\\':
			case '|':
			case '(':
			case ')':
			case '?':
			case '+':
			case '*':
			case '.':
			case '^':
			case '$':
				str.push_back('\\');
				break;
			}
			str.push_back(c);
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto cc = soup::make_unique<RegexCharConstraint>(c);
			success_transitions.setTransitionTo(cc->getEntrypoint());
			success_transitions.emplace(&cc->success_transition);
			return cc;
		}
	};
}
