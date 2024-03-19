#pragma once

#include "RngInterface.hpp"

namespace soup
{
	struct DefaultRngInterface : public StatelessRngInterface
	{
		[[nodiscard]] uint64_t generate() final;
	};
}
