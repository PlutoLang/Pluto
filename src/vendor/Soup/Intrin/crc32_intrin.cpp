#include <cstddef>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
	#include <smmintrin.h> // _mm_extract_epi32
	#include <wmmintrin.h> // _mm_clmulepi64_si128
#elif defined(__aarch64__) || defined(_M_ARM64)
	#ifdef _WIN32
		#include <intrin.h>
	#else
		#include <arm_acle.h>
	#endif
#endif

namespace soup_intrin
{
#if defined(__x86_64__) || defined(_M_X64)
	uint32_t crc32_pclmul(const uint8_t* p, size_t size, uint32_t crc) noexcept
	{
		// Original source: https://github.com/richgel999/fpng/blob/main/src/fpng.cpp
		// Original licence: Dedicated to the public domain.

		static const uint64_t
#ifdef _MSC_VER
			__declspec(align(16))
#else
			__attribute__((aligned(16)))
#endif
			s_u[2] = { 0x1DB710641, 0x1F7011641 }, s_k5k0[2] = { 0x163CD6124, 0 }, s_k3k4[2] = { 0x1751997D0, 0xCCAA009E };

		// Load first 16 bytes, apply initial CRC32
		__m128i b = _mm_xor_si128(_mm_cvtsi32_si128(~crc), _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)));

		// We're skipping directly to Step 2 page 12 - iteratively folding by 1 (by 4 is overkill for our needs)
		const __m128i k3k4 = _mm_load_si128(reinterpret_cast<const __m128i*>(s_k3k4));

		for (size -= 16, p += 16; size >= 16; size -= 16, p += 16)
			b = _mm_xor_si128(_mm_xor_si128(_mm_clmulepi64_si128(b, k3k4, 17), _mm_loadu_si128(reinterpret_cast<const __m128i*>(p))), _mm_clmulepi64_si128(b, k3k4, 0));

		// Final stages: fold to 64-bits, 32-bit Barrett reduction
		const __m128i z = _mm_set_epi32(0, ~0, 0, ~0), u = _mm_load_si128(reinterpret_cast<const __m128i*>(s_u));
		b = _mm_xor_si128(_mm_srli_si128(b, 8), _mm_clmulepi64_si128(b, k3k4, 16));
		b = _mm_xor_si128(_mm_clmulepi64_si128(_mm_and_si128(b, z), _mm_loadl_epi64(reinterpret_cast<const __m128i*>(s_k5k0)), 0), _mm_srli_si128(b, 4));
		return ~_mm_extract_epi32(_mm_xor_si128(b, _mm_clmulepi64_si128(_mm_and_si128(_mm_clmulepi64_si128(_mm_and_si128(b, z), u, 16), z), u, 0)), 1);
	}
#elif defined(__aarch64__) || defined(_M_ARM64)
	uint32_t crc32_armv8(const uint8_t* p, size_t size, uint32_t crc) noexcept
	{
		crc = ~crc;
		for (; size >= 8; size -= 8)
		{
			crc = __crc32d(crc, *reinterpret_cast<const uint64_t*>(p));
			p += 8;
		}
		while (size--)
		{
			crc = __crc32b(crc, *p++);
		}
		crc = ~crc;
		return crc;
	}
#endif
}
