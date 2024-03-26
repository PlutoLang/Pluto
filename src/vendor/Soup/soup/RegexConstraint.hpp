#pragma once

#include <string>

#include "fwd.hpp"

#include "Exception.hpp"

NAMESPACE_SOUP
{
	struct RegexConstraint
	{
		inline static RegexConstraint* SUCCESS_TO_FAIL = reinterpret_cast<RegexConstraint*>(0b100);
		inline static RegexConstraint* ROLLBACK_TO_SUCCESS = reinterpret_cast<RegexConstraint*>(0b100);
		inline static uintptr_t MASK = 0b11;

		RegexConstraint* success_transition = nullptr;
		RegexConstraint* rollback_transition = nullptr;
		const RegexGroup* group = nullptr;

		RegexConstraint() = default;

		RegexConstraint(const RegexConstraint& b)
		{
			// We want the pointers to be nullptr so transitions are not copied by `clone`.
		}

		virtual ~RegexConstraint() = default;

		[[nodiscard]] RegexConstraint* getSuccessTransition() const noexcept
		{
			return reinterpret_cast<RegexConstraint*>(reinterpret_cast<uintptr_t>(success_transition) & ~MASK);
		}

		[[nodiscard]] RegexConstraint* getRollbackTransition() const noexcept
		{
			return reinterpret_cast<RegexConstraint*>(reinterpret_cast<uintptr_t>(rollback_transition) & ~MASK);
		}

		[[nodiscard]] virtual bool shouldResetCapture() const noexcept
		{
			return false;
		}

		// May only modify `m.it` and only if the constraint matches.
		[[nodiscard]] virtual bool matches(RegexMatcher& m) const noexcept = 0;

		[[nodiscard]] virtual RegexConstraint* getEntrypoint() noexcept
		{
			return this;
		}

		[[nodiscard]] virtual const RegexGroup* getGroupCaturedWithin() const noexcept
		{
			return group;
		}

		[[nodiscard]] virtual size_t getCursorAdvancement() const
		{
			SOUP_THROW(Exception("Constraint is not fixed-width"));
		}

		[[nodiscard]] virtual UniquePtr<RegexConstraint> clone(RegexTransitionsVector& success_transitions) const
		{
			SOUP_THROW(Exception("Constraint is not clonable"));
		}

		[[nodiscard]] virtual std::string toString() const noexcept = 0;

		virtual void getFlags(uint16_t& set, uint16_t& unset) const noexcept {}
	};
}
