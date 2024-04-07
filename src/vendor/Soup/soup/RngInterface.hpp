#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct RngInterface
	{
		[[nodiscard]] virtual uint64_t generate() = 0;

		[[nodiscard]] bool coinflip()
		{
			return generate() & 1;
		}
	};

	struct StatelessRngInterface : public RngInterface
	{
	};
}
