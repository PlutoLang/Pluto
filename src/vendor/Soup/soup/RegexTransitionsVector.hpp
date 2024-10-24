#pragma once

#include <vector>

NAMESPACE_SOUP
{
	struct RegexTransitionsVector
	{
		std::vector<RegexConstraint**> data;
		std::vector<RegexConstraint**> prev_data;

		void emplace(RegexConstraint** p)
		{
			data.emplace_back(p);
		}

		void emplaceRollback(RegexConstraint** p)
		{
			data.emplace_back(p);

			// If we don't have a next constraint, rollback is match success.
			*p = RegexConstraint::ROLLBACK_TO_SUCCESS;
		}

		void setPreviousTransitionTo(RegexConstraint* c) noexcept
		{
			SOUP_ASSERT((reinterpret_cast<uintptr_t>(c) & RegexConstraint::MASK) == 0);

			for (const auto& p : prev_data)
			{
				*p = reinterpret_cast<RegexConstraint*>(reinterpret_cast<uintptr_t>(c) | (reinterpret_cast<uintptr_t>(*p) & 0b10));
			}
		}

		void setResetCapture() noexcept
		{
			for (const auto& p : data)
			{
				*reinterpret_cast<uintptr_t*>(p) = 0b10;
			}
		}

		void setTransitionTo(RegexConstraint* c, bool save_checkpoint = false) noexcept
		{
			SOUP_ASSERT((reinterpret_cast<uintptr_t>(c) & RegexConstraint::MASK) == 0);

			if (save_checkpoint)
			{
				reinterpret_cast<uintptr_t&>(c) |= 0b1;
			}

			for (const auto& p : data)
			{
				*p = reinterpret_cast<RegexConstraint*>(reinterpret_cast<uintptr_t>(c) | (reinterpret_cast<uintptr_t>(*p) & 0b10));
			}

			prev_data = std::move(data);
			data.clear();
		}

		void discharge(std::vector<RegexConstraint**>& outTransitions) noexcept
		{
			for (const auto& p : data)
			{
				outTransitions.emplace_back(p);
			}
			data.clear();
		}

		void rollback() noexcept
		{
			data = std::move(prev_data);
			prev_data.clear();

			for (const auto& p : data)
			{
				*reinterpret_cast<uintptr_t*>(p) &= 0b10;
			}
		}
	};
}
