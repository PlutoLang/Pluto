#include "rand.hpp"

#include "HardwareRng.hpp"

NAMESPACE_SOUP
{
	uint64_t rand_impl::getSeed() noexcept
	{
		return FastHardwareRng::generate64();
	}

	uint8_t rand_impl::byte(uint8_t min) noexcept
	{
		return t<uint8_t>(min, -1);
	}

	std::string rand_impl::binstr(size_t len)
	{
		std::string str{};
		for (size_t i = 0; i < len; i++)
		{
			str.push_back(byte());
		}
		return str;
	}
}
