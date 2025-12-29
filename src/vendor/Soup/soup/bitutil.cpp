#include "bitutil.hpp"

#if SOUP_X86
#include <immintrin.h>

#include "CpuInfo.hpp"
#endif

NAMESPACE_SOUP
{
#if SOUP_X86
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("bmi2")))
	#endif
	static uint32_t _pdep_u32_wrapper(uint32_t src, uint32_t mask)
	{
		return _pdep_u32(src, mask);
	}
#endif

	uint32_t bitutil::parallelDeposit(uint32_t src, uint32_t mask)
	{
#if SOUP_X86
		if (CpuInfo::get().supportsBMI2())
		{
			return _pdep_u32_wrapper(src, mask);
		}
#endif
		uint32_t dest = 0;
		uint32_t m = 0;
		uint32_t k = 0;
		for (; m != 32; ++m)
		{
			if ((mask >> m) & 1)
			{
				dest |= ((src >> k++) & 1) << m;
			}
		}
		return dest;
	}

#if SOUP_X86 && SOUP_BITS == 64
	#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("bmi2")))
	#endif
	static uint64_t _pdep_u64_wrapper(uint64_t src, uint64_t mask)
	{
		return _pdep_u64(src, mask);
	}
#endif

	uint64_t bitutil::parallelDeposit(uint64_t src, uint64_t mask)
	{
#if SOUP_X86 && SOUP_BITS == 64
		if (CpuInfo::get().supportsBMI2())
		{
			return _pdep_u64_wrapper(src, mask);
		}
#endif
		uint64_t dest = 0;
		uint32_t m = 0;
		uint32_t k = 0;
		for (; m != 64; ++m)
		{
			if ((mask >> m) & 1)
			{
				dest |= ((src >> k++) & 1) << m;
			}
		}
		return dest;
	}

	std::vector<bool> bitutil::interleave(const std::vector<std::vector<bool>>& data)
	{
		const auto inner_size = data.at(0).size();
		const auto outer_size = data.size();
		std::vector<bool> out(inner_size * outer_size, false);
		size_t outer_i = 0;
		size_t inner_i = 0;
		for (size_t i = 0;; ++i)
		{
			out.at(i) = data.at(outer_i).at(inner_i);
			if (++outer_i == outer_size)
			{
				outer_i = 0;
				if (++inner_i == inner_size)
				{
					break;
				}
			}
		}
		return out;
	}
}
