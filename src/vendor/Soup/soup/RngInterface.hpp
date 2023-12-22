#pragma once

#include <cstdint>

namespace soup
{
	struct RngInterface
	{
		[[nodiscard]] virtual uint64_t generate() = 0;

		[[nodiscard]] bool coinflip()
		{
			return generate() & 1;
		}
	};
}
