#include "rand.hpp"

namespace soup
{
	uint8_t rand_impl::byte(uint8_t min) noexcept
	{
		return t<uint8_t>(min, -1);
	}
}
