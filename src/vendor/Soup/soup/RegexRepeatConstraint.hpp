#pragma once

#include "RegexConstraint.hpp"

#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	template <bool at_least_one, bool greedy>
	struct RegexRepeatConstraint : public RegexConstraint
	{
		UniquePtr<RegexConstraint> constraint;

		RegexRepeatConstraint(UniquePtr<RegexConstraint>&& constraint)
			: constraint(std::move(constraint))
		{
		}

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			// Meta-constraint. Transitions will be set up to correctly handle matching of this.
			return true;
		}

		[[nodiscard]] virtual RegexConstraint* getEntrypoint() noexcept final
		{
			return constraint->getEntrypoint();
		}

		void setupTransitionsAtLeastOne(RegexTransitionsVector& success_transitions)
		{
			success_transitions.setTransitionTo(this);
			if (greedy)
			{
				// quantifier --[success]-> constraint
				success_transitions.emplace(&success_transition);
				if (constraint->shouldResetCapture())
				{
					success_transitions.setResetCapture();
				}
				success_transitions.setTransitionTo(constraint->getEntrypoint());

				// quantifier --[rollback]-> next-constraint
				success_transitions.emplaceRollback(&rollback_transition);
			}
			else
			{
				// quantifier --[success]-> next-constraint
				success_transitions.emplace(&success_transition);
				if (constraint->shouldResetCapture())
				{
					success_transitions.setResetCapture();
				}

				// quantifier --[rollback]-> constraint
				rollback_transition = constraint->getEntrypoint();
			}
		}

		[[nodiscard]] UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const final
		{
			if (at_least_one)
			{
				auto cc = soup::make_unique<RegexRepeatConstraint>(constraint->clone(success_transitions));
				cc->constraint->group = constraint->group;
				cc->setupTransitionsAtLeastOne(success_transitions);
				return cc;
			}
			return RegexConstraint::clone(success_transitions);
		}

		[[nodiscard]] std::string toString() const noexcept final
		{
			std::string str = constraint->toString();
			if (at_least_one)
			{
				str.push_back('+');
			}
			else
			{
				str.push_back('*');
			}
			if (!greedy)
			{
				str.push_back('?');
			}
			return str;
		}
	};
}
