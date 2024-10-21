#pragma once

#include "RegexConstraint.hpp"

#include "RegexMatcher.hpp"

NAMESPACE_SOUP
{
	template <bool dotall, bool unicode>
	struct RegexAnyCharConstraint : public RegexConstraint
	{
		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			if (m.it == m.end)
			{
				return false;
			}
			if constexpr (!dotall)
			{
				if (*m.it == '\n')
				{
					return false;
				}
			}
			if constexpr (unicode)
			{
				unicode::utf8_add(m.it, m.end);
			}
			else
			{
				++m.it;
			}
			return true;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			return ".";
		}

		void getFlags(uint16_t& set, uint16_t& unset) const noexcept final
		{
			if constexpr (dotall)
			{
				set |= RE_DOTALL;
			}
			else
			{
				unset |= RE_DOTALL;
			}
			if constexpr (unicode)
			{
				set |= RE_UNICODE;
			}
			else
			{
				unset |= RE_UNICODE;
			}
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return 1;
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto cc = soup::make_unique<RegexAnyCharConstraint>();
			success_transitions.setTransitionTo(cc->getEntrypoint());
			success_transitions.emplace(&cc->success_transition);
			return cc;
		}
	};
}
