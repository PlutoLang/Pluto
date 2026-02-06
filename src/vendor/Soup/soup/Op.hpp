#pragma once

#include <vector>

#include "base.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct Op
	{
		uint8_t type = 0xFF;
		std::vector<UniquePtr<astNode>> args{};
	};
}
