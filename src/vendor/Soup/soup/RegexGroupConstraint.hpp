#pragma once

#include "RegexConstraint.hpp"

#include "RegexGroup.hpp"

#include <unordered_map>

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

		[[nodiscard]] const RegexGroup* getGroupCaturedWithin() const noexcept final
		{
			return &data;
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			auto str = data.toString();
			if (data.isNonCapturing())
			{
				str.insert(0, "?:");
			}
			else if (!data.name.empty())
			{
				if (data.name.find('\'') != std::string::npos)
				{
					str.insert(0, 1, '>');
					str.insert(0, data.name);
					str.insert(0, 1, '<');
				}
				else
				{
					str.insert(0, 1, '\'');
					str.insert(0, data.name);
					str.insert(0, 1, '\'');
				}
				str.insert(0, 1, '?');
			}
			str.insert(0, 1, '(');
			str.push_back(')');
			return str;
		}

		void getFlags(uint16_t& set, uint16_t& unset) const noexcept final
		{
			data.getFlags(set, unset);
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
