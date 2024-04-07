#include "DefaultRngInterface.hpp"

#include "rand.hpp"

NAMESPACE_SOUP
{
	uint64_t DefaultRngInterface::generate()
	{
		return rand.t<uint64_t>(0, -1);
	}
}
