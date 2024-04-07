#pragma once

#include "RngInterface.hpp"

NAMESPACE_SOUP
{
	struct DefaultRngInterface : public StatelessRngInterface
	{
		[[nodiscard]] uint64_t generate() final;
	};
}
