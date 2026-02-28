#pragma once

#include "RegexConstraint.hpp"

#include <unordered_map>

#include "RegexGroup.hpp"
#include "RegexTransitionsVector.hpp"

NAMESPACE_SOUP
{
	struct RegexGroupConstraint : public RegexConstraint
	{
		RegexGroup data;

		RegexGroupConstraint(size_t index)
			: data(index)
		{
		}

		RegexGroupConstraint(const RegexGroup::ConstructorState& s, bool non_capturing)
			: data(s, non_capturing)
		{
		}

		[[nodiscard]] bool shouldResetCapture() const noexcept final
		{
			return !data.isNonCapturing();
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final;

		[[nodiscard]] const RegexGroup* getGroupCaturedWithin() const noexcept final
		{
			return &data;
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return data.getCursorAdvancement();
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			auto upClone = soup::make_unique<RegexGroupConstraint>(data.index);
			success_transitions.setTransitionTo(upClone.get());
			success_transitions.emplace(&upClone->success_transition);
			for (const auto& a : data.alternatives)
			{
				RegexAlternative& ac = upClone->data.alternatives.emplace_back();
				for (const auto& c : a.constraints)
				{
					auto pConstraintClone = ac.constraints.emplace_back(c->clone(success_transitions)).get();
					pConstraintClone->group = &upClone->data;
					if (!upClone->data.initial)
					{
						if (data.initial == c.get())
						{
							upClone->data.initial = pConstraintClone;
						}
						else if (data.initial == c->getEntrypoint())
						{
							upClone->data.initial = pConstraintClone->getEntrypoint();
						}
					}
				}
			}

			upClone->data.parent = data.parent;
			upClone->data.name = data.name;
			upClone->data.lookahead_or_lookbehind = data.lookahead_or_lookbehind;

			SOUP_ASSERT(upClone->data.initial, "Failed to find initial constraint for cloned group");

			return upClone;
		}
	};
}
