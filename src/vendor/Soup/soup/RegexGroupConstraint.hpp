#pragma once

#include "RegexConstraint.hpp"

#include "RegexGroup.hpp"

#include <unordered_map>

namespace soup
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

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}

		[[nodiscard]] const RegexConstraint* getEntrypoint() const noexcept final
		{
			return data.initial;
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

		[[nodiscard]] UniquePtr<RegexConstraint> clone() const final
		{
			auto upClone = soup::make_unique<RegexGroupConstraint>(data.index);
			std::unordered_map<const RegexConstraint*, const RegexConstraint*> constraint_clones{};
			for (const auto& a : data.alternatives)
			{
				RegexAlternative& ac = upClone->data.alternatives.emplace_back();
				for (const auto& c : a.constraints)
				{
					auto pConstraintClone = ac.constraints.emplace_back(c->clone()).get();
					pConstraintClone->success_transition = c->success_transition;
					pConstraintClone->rollback_transition = c->rollback_transition;
					pConstraintClone->group.set(&upClone->data, c->group.getBool());
					constraint_clones.emplace(c.get(), pConstraintClone);
				}
			}

			upClone->data.parent = data.parent;
			if (auto e = constraint_clones.find(data.initial); e != constraint_clones.end())
			{
				upClone->data.initial = static_cast<const RegexConstraint*>(e->second);
			}
			for (const auto& a : upClone->data.alternatives)
			{
				for (auto& c : a.constraints)
				{
					if (auto e = constraint_clones.find(c->success_transition); e != constraint_clones.end())
					{
						c->success_transition = static_cast<const RegexConstraint*>(e->second);
					}
					if (auto e = constraint_clones.find(c->rollback_transition); e != constraint_clones.end())
					{
						c->rollback_transition = static_cast<const RegexConstraint*>(e->second);
					}
				}
			}
			upClone->data.name = data.name;
			upClone->data.lookahead_or_lookbehind = data.lookahead_or_lookbehind;

			upClone->data.alternatives.back().constraints.back()->success_transition = upClone.get();

			return upClone;
		}
	};
}
