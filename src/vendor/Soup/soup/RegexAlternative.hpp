#pragma once

#include <vector>

#include "UniquePtr.hpp"
#include "RegexConstraint.hpp"

namespace soup
{
	struct RegexAlternative
	{
		std::vector<UniquePtr<RegexConstraint>> constraints{};
	};
}
