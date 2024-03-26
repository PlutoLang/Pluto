#include "DefaultRngInterface.hpp"

#include "rand.hpp"

namespace soup
{
	uint64_t DefaultRngInterface::generate()
	{
		return rand.t<uint64_t>(0, -1);
	}
}
