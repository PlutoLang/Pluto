#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

#include <cstdint>

#include <immintrin.h>

namespace soup_intrin
{
	// RDSEED

	uint16_t hardware_rng_generate16() noexcept
	{
		uint16_t res;
		while (_rdseed16_step(&res) == 0);
		return res;
	}

	uint32_t hardware_rng_generate32() noexcept
	{
		uint32_t res;
		while (_rdseed32_step(&res) == 0);
		return res;
	}

#if defined(__x86_64__) || defined(_M_X64)
	uint64_t hardware_rng_generate64() noexcept
	{
		unsigned long long res;
		while (_rdseed64_step(&res) == 0);
		return res;
	}
	static_assert(sizeof(uint64_t) == sizeof(unsigned long long));
#endif

	// RDRAND

	uint16_t fast_hardware_rng_generate16() noexcept
	{
		uint16_t res;
		while (_rdrand16_step(&res) == 0);
		return res;
	}

	uint32_t fast_hardware_rng_generate32() noexcept
	{
		uint32_t res;
		while (_rdrand32_step(&res) == 0);
		return res;
	}

#if defined(__x86_64__) || defined(_M_X64)
	uint64_t fast_hardware_rng_generate64() noexcept
	{
		unsigned long long res;
		while (_rdrand64_step(&res) == 0);
		return res;
	}
	static_assert(sizeof(uint64_t) == sizeof(unsigned long long));
#endif
}

#endif
