#pragma once

#include "RegexConstraint.hpp"

#include <vector>

#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct RegexRangeQuantifierConstraintBase : public RegexConstraint
	{
		std::vector<UniquePtr<RegexConstraint>> constraints;
		size_t min_reps;

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}

		[[nodiscard]] RegexConstraint* getEntrypoint() noexcept final
		{
			return constraints.at(0)->getEntrypoint();
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return constraints.at(0)->getCursorAdvancement() * constraints.size();
		}
	};

	struct RegexRangeQuantifierConstraintGreedy : public RegexRangeQuantifierConstraintBase
	{
		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			constraints.at(0)->toString(str, flags);
			str.push_back('{');
			str.append(std::to_string(min_reps));
			str.push_back(',');
			str.append(std::to_string(constraints.size()));
			str.push_back('}');
		}
	};

	struct RegexRangeQuantifierConstraintLazy : public RegexRangeQuantifierConstraintBase
	{
		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			const size_t optional_reps = (constraints.size() - min_reps) / 2;

			constraints.at(0)->toString(str, flags);
			str.push_back('{');
			str.append(std::to_string(min_reps));
			str.push_back(',');
			str.append(std::to_string(min_reps + optional_reps));
			str.push_back('}');
			str.push_back('?');
		}
	};
}
