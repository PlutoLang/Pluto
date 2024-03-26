#pragma once

#include <string>

#include "fwd.hpp"

#include "Exception.hpp"
#include "PointerAndBool.hpp"

namespace soup
{
	struct RegexConstraint
	{
		const RegexConstraint* success_transition = nullptr;
		const RegexConstraint* rollback_transition = nullptr;
		PointerAndBool<const RegexGroup*> group = nullptr;

		RegexConstraint() = default;

		RegexConstraint(const RegexConstraint& b)
		{
			// We want the pointers to be nullptr so transitions are not copied by `clone`.
		}

		virtual ~RegexConstraint() = default;

		// May only modify `m.it` and only if the constraint matches.
		[[nodiscard]] virtual bool matches(RegexMatcher& m) const noexcept = 0;

		[[nodiscard]] virtual const RegexConstraint* getEntrypoint() const noexcept
		{
			return this;
		}

		[[nodiscard]] virtual size_t getCursorAdvancement() const
		{
			SOUP_THROW(Exception("Constraint is not fixed-width"));
		}

		[[nodiscard]] virtual UniquePtr<RegexConstraint> clone() const
		{
			SOUP_THROW(Exception("Constraint is not clonable"));
		}

		[[nodiscard]] virtual std::string toString() const noexcept = 0;

		virtual void getFlags(uint16_t& set, uint16_t& unset) const noexcept {}
	};
}
