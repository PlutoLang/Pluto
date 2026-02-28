#pragma once

#include "RegexConstraint.hpp"

#include <vector>

#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct RegexExactQuantifierConstraint : public RegexConstraint
	{
		std::vector<UniquePtr<RegexConstraint>> constraints;

		[[nodiscard]] bool matches(RegexMatcher& m) const noexcept final
		{
			return true;
		}

		[[nodiscard]] RegexConstraint* getEntrypoint() noexcept final
		{
			return constraints.at(0)->getEntrypoint();
		}

		void toString(std::string& str, uint16_t& flags) const SOUP_EXCAL final
		{
			constraints.at(0)->toString(str, flags);
			str.push_back('{');
			str.append(std::to_string(constraints.size()));
			str.push_back('}');
		}

		[[nodiscard]] size_t getCursorAdvancement() const final
		{
			return constraints.at(0)->getCursorAdvancement() * constraints.size();
		}
	};
}
