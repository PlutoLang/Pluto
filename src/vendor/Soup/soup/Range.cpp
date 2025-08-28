#include "Range.hpp"

#include "base.hpp"

#if SOUP_X86 && SOUP_BITS == 64
#include <emmintrin.h>
#include <immintrin.h>
#endif

#if SOUP_WINDOWS
#include <windows.h>
#endif

#include "bitutil.hpp"
#include "CpuInfo.hpp"
#include "Pattern.hpp"

NAMESPACE_SOUP
{
	bool Range::pattern_matches(uint8_t* target, const std::optional<uint8_t>* sig, size_t length) noexcept
	{
#if SOUP_WINDOWS && !SOUP_CROSS_COMPILE
		__try
		{
#endif
			for (size_t i = 0; i < length; ++i)
			{
				if (sig[i] && *sig[i] != target[i])
				{
					return false;
				}
			}
			return true;
#if SOUP_WINDOWS && !SOUP_CROSS_COMPILE
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return false;
#endif
	}

	size_t Range::scanWithMultipleResults(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept
	{
		const auto data = sig.bytes.data();
		const auto length = sig.bytes.size();
		SOUP_IF_UNLIKELY (length > this->size)
		{
			if (length == 0)
			{
				buf[0] = base;
				return 1;
			}
			return 0;
		}
#if SOUP_X86 && SOUP_BITS == 64
		SOUP_IF_LIKELY (data[sig.most_unique_byte_index].has_value())
		{
			const CpuInfo& cpuinfo = CpuInfo::get();
			if (cpuinfo.supportsAVX512F() && cpuinfo.supportsAVX512BW())
			{
				return scanWithMultipleResultsAvx512(sig, buf, buflen);
			}
			if (cpuinfo.supportsAVX2())
			{
				return scanWithMultipleResultsAvx2(sig, buf, buflen);
			}
			if (cpuinfo.supportsSSE2())
			{
				return scanWithMultipleResultsSimd(sig, buf, buflen);
			}
		}
#endif
		size_t accum = 0;
		for (uintptr_t i = 0; i != (size - length); ++i)
		{
			if (pattern_matches(base.add(i).as<uint8_t*>(), data, length))
			{
				buf[accum++] = base.add(i);
				if (accum == buflen)
				{
					break;
				}
			}
		}
		return accum;
	}

#if SOUP_X86 && SOUP_BITS == 64
	size_t Range::scanWithMultipleResultsSimd(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept
	{
		const auto data = sig.bytes.data();
		const auto length = sig.bytes.size();
		const auto match = _mm_set1_epi8(*data[sig.most_unique_byte_index]);
		size_t accum = 0;
		for (uintptr_t i = sig.most_unique_byte_index; i < (size - length - 15); i += 16)
		{
			uint32_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(match, _mm_loadu_si128(base.add(i).as<const __m128i*>())));
			while (mask != 0)
			{
				const auto j = bitutil::getLeastSignificantSetBit(mask);
				if (pattern_matches(base.add(i).add(j).sub(sig.most_unique_byte_index).as<uint8_t*>(), data, length))
				{
					buf[accum++] = base.add(i).add(j).sub(sig.most_unique_byte_index);
					if (accum == buflen)
					{
						return buflen;
					}
				}
				bitutil::unsetLeastSignificantSetBit(mask);
			}
		}
		return accum;
	}
#endif

#if SOUP_X86 && SOUP_BITS == 64
#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("avx2")))
#endif
	size_t Range::scanWithMultipleResultsAvx2(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept
	{
		const auto data = sig.bytes.data();
		const auto length = sig.bytes.size();
		const auto match = _mm256_set1_epi8(*data[sig.most_unique_byte_index]);
		size_t accum = 0;
		for (uintptr_t i = sig.most_unique_byte_index; i < (size - length - 31); i += 32)
		{
			uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(match, _mm256_loadu_si256(base.add(i).as<const __m256i*>())));
			while (mask != 0)
			{
				const auto j = bitutil::getLeastSignificantSetBit(mask);
				if (pattern_matches(base.add(i).add(j).sub(sig.most_unique_byte_index).as<uint8_t*>(), data, length))
				{
					buf[accum++] = base.add(i).add(j).sub(sig.most_unique_byte_index);
					if (accum == buflen)
					{
						return buflen;
					}
				}
				bitutil::unsetLeastSignificantSetBit(mask);
			}
		}
		return accum;
	}

#if defined(__GNUC__) || defined(__clang__)
	__attribute__((target("avx512f,avx512bw")))
#endif
	size_t Range::scanWithMultipleResultsAvx512(const Pattern& sig, Pointer buf[], size_t buflen) const noexcept
	{
		const auto data = sig.bytes.data();
		const auto length = sig.bytes.size();
		const auto match = _mm512_set1_epi8(*data[sig.most_unique_byte_index]);
		size_t accum = 0;
		for (uintptr_t i = sig.most_unique_byte_index; i < (size - length - 63); i += 64)
		{
			uint64_t mask = _mm512_cmpeq_epi8_mask(match, _mm512_loadu_si512(base.add(i).as<const __m512i*>()));
			while (mask != 0)
			{
				const auto j = bitutil::getLeastSignificantSetBit(mask);
				if (pattern_matches(base.add(i).add(j).sub(sig.most_unique_byte_index).as<uint8_t*>(), data, length))
				{
					buf[accum++] = base.add(i).add(j).sub(sig.most_unique_byte_index);
					if (accum == buflen)
					{
						return buflen;
					}
				}
				bitutil::unsetLeastSignificantSetBit(mask);
			}
		}
		return accum;
	}
#endif
}
