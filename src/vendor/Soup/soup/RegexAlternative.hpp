#pragma once

#include <vector>

#include "UniquePtr.hpp"
#include "RegexConstraint.hpp"

NAMESPACE_SOUP
{
	struct RegexAlternative
	{
		std::vector<UniquePtr<RegexConstraint>> constraints{};
	};
}
