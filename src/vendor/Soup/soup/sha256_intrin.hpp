// THIS FILE IS FOR INTERNAL USE ONLY. DO NOT INCLUDE THIS IN YOUR OWN CODE.

#include "base.hpp"

#include <cstdint>

#if SOUP_X86
	#include <immintrin.h>
#elif SOUP_ARM
	#include <arm_neon.h>
#endif

NAMESPACE_SOUP
{
	namespace intrin
	{
		// Original source: https://github.com/noloader/SHA-Intrinsics
		// Original licence: Dedicated to the public domain.

#if SOUP_X86
	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("sha,sse4.1")))
	#endif
		void sha256_transform(uint32_t state[8], const uint8_t data[64]) noexcept
		{
			__m128i STATE0, STATE1;
			__m128i MSG, TMP;
			__m128i MSG0, MSG1, MSG2, MSG3;
			__m128i ABEF_SAVE, CDGH_SAVE;
			const __m128i MASK = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);

			/* Load initial values */
			TMP = _mm_loadu_si128((const __m128i*) & state[0]);
			STATE1 = _mm_loadu_si128((const __m128i*) & state[4]);

			TMP = _mm_shuffle_epi32(TMP, 0xB1);          /* CDAB */
			STATE1 = _mm_shuffle_epi32(STATE1, 0x1B);    /* EFGH */
			STATE0 = _mm_alignr_epi8(TMP, STATE1, 8);    /* ABEF */
			STATE1 = _mm_blend_epi16(STATE1, TMP, 0xF0); /* CDGH */

			/* Save current state */
			ABEF_SAVE = STATE0;
			CDGH_SAVE = STATE1;

			/* Rounds 0-3 */
			MSG = _mm_loadu_si128((const __m128i*) (data + 0));
			MSG0 = _mm_shuffle_epi8(MSG, MASK);
			MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

			/* Rounds 4-7 */
			MSG1 = _mm_loadu_si128((const __m128i*) (data + 16));
			MSG1 = _mm_shuffle_epi8(MSG1, MASK);
			MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

			/* Rounds 8-11 */
			MSG2 = _mm_loadu_si128((const __m128i*) (data + 32));
			MSG2 = _mm_shuffle_epi8(MSG2, MASK);
			MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

			/* Rounds 12-15 */
			MSG3 = _mm_loadu_si128((const __m128i*) (data + 48));
			MSG3 = _mm_shuffle_epi8(MSG3, MASK);
			MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
			MSG0 = _mm_add_epi32(MSG0, TMP);
			MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

			/* Rounds 16-19 */
			MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
			MSG1 = _mm_add_epi32(MSG1, TMP);
			MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

			/* Rounds 20-23 */
			MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
			MSG2 = _mm_add_epi32(MSG2, TMP);
			MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

			/* Rounds 24-27 */
			MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
			MSG3 = _mm_add_epi32(MSG3, TMP);
			MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

			/* Rounds 28-31 */
			MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x1429296706CA6351ULL, 0xD5A79147C6E00BF3ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
			MSG0 = _mm_add_epi32(MSG0, TMP);
			MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

			/* Rounds 32-35 */
			MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
			MSG1 = _mm_add_epi32(MSG1, TMP);
			MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

			/* Rounds 36-39 */
			MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
			MSG2 = _mm_add_epi32(MSG2, TMP);
			MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);

			/* Rounds 40-43 */
			MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
			MSG3 = _mm_add_epi32(MSG3, TMP);
			MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);

			/* Rounds 44-47 */
			MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
			MSG0 = _mm_add_epi32(MSG0, TMP);
			MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);

			/* Rounds 48-51 */
			MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
			MSG1 = _mm_add_epi32(MSG1, TMP);
			MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
			MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);

			/* Rounds 52-55 */
			MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x682E6FF35B9CCA4FULL, 0x4ED8AA4A391C0CB3ULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
			MSG2 = _mm_add_epi32(MSG2, TMP);
			MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

			/* Rounds 56-59 */
			MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
			MSG3 = _mm_add_epi32(MSG3, TMP);
			MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

			/* Rounds 60-63 */
			MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
			STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
			MSG = _mm_shuffle_epi32(MSG, 0x0E);
			STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);

			/* Combine state  */
			STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
			STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);

			TMP = _mm_shuffle_epi32(STATE0, 0x1B);       /* FEBA */
			STATE1 = _mm_shuffle_epi32(STATE1, 0xB1);    /* DCHG */
			STATE0 = _mm_blend_epi16(TMP, STATE1, 0xF0); /* DCBA */
			STATE1 = _mm_alignr_epi8(STATE1, TMP, 8);    /* ABEF */

			/* Save state */
			_mm_storeu_si128((__m128i*) & state[0], STATE0);
			_mm_storeu_si128((__m128i*) & state[4], STATE1);
		}
#elif SOUP_ARM
		inline const uint32_t sha256_k[8 * 8] = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
			0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
			0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
			0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
			0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
			0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
			0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
			0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
			0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
		};

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void sha256_transform(uint32_t state[8], const uint8_t data[64]) noexcept
		{
			uint32x4_t STATE0, STATE1, ABEF_SAVE, CDGH_SAVE;
			uint32x4_t MSG0, MSG1, MSG2, MSG3;
			uint32x4_t TMP0, TMP1, TMP2;

			/* Load state */
			STATE0 = vld1q_u32(&state[0]);
			STATE1 = vld1q_u32(&state[4]);

			/* Save state */
			ABEF_SAVE = STATE0;
			CDGH_SAVE = STATE1;

			/* Load message */
			MSG0 = vld1q_u32((const uint32_t*)(data + 0));
			MSG1 = vld1q_u32((const uint32_t*)(data + 16));
			MSG2 = vld1q_u32((const uint32_t*)(data + 32));
			MSG3 = vld1q_u32((const uint32_t*)(data + 48));

			/* Reverse for little endian */
			MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG0)));
			MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG1)));
			MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG2)));
			MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG3)));

			TMP0 = vaddq_u32(MSG0, vld1q_u32(&sha256_k[0x00]));

			/* Rounds 0-3 */
			MSG0 = vsha256su0q_u32(MSG0, MSG1);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG1, vld1q_u32(&sha256_k[0x04]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

			/* Rounds 4-7 */
			MSG1 = vsha256su0q_u32(MSG1, MSG2);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG2, vld1q_u32(&sha256_k[0x08]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

			/* Rounds 8-11 */
			MSG2 = vsha256su0q_u32(MSG2, MSG3);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG3, vld1q_u32(&sha256_k[0x0c]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

			/* Rounds 12-15 */
			MSG3 = vsha256su0q_u32(MSG3, MSG0);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG0, vld1q_u32(&sha256_k[0x10]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

			/* Rounds 16-19 */
			MSG0 = vsha256su0q_u32(MSG0, MSG1);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG1, vld1q_u32(&sha256_k[0x14]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

			/* Rounds 20-23 */
			MSG1 = vsha256su0q_u32(MSG1, MSG2);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG2, vld1q_u32(&sha256_k[0x18]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

			/* Rounds 24-27 */
			MSG2 = vsha256su0q_u32(MSG2, MSG3);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG3, vld1q_u32(&sha256_k[0x1c]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

			/* Rounds 28-31 */
			MSG3 = vsha256su0q_u32(MSG3, MSG0);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG0, vld1q_u32(&sha256_k[0x20]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

			/* Rounds 32-35 */
			MSG0 = vsha256su0q_u32(MSG0, MSG1);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG1, vld1q_u32(&sha256_k[0x24]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

			/* Rounds 36-39 */
			MSG1 = vsha256su0q_u32(MSG1, MSG2);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG2, vld1q_u32(&sha256_k[0x28]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

			/* Rounds 40-43 */
			MSG2 = vsha256su0q_u32(MSG2, MSG3);
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG3, vld1q_u32(&sha256_k[0x2c]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);
			MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

			/* Rounds 44-47 */
			MSG3 = vsha256su0q_u32(MSG3, MSG0);
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG0, vld1q_u32(&sha256_k[0x30]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);
			MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

			/* Rounds 48-51 */
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG1, vld1q_u32(&sha256_k[0x34]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

			/* Rounds 52-55 */
			TMP2 = STATE0;
			TMP0 = vaddq_u32(MSG2, vld1q_u32(&sha256_k[0x38]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

			/* Rounds 56-59 */
			TMP2 = STATE0;
			TMP1 = vaddq_u32(MSG3, vld1q_u32(&sha256_k[0x3c]));
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP0);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP0);

			/* Rounds 60-63 */
			TMP2 = STATE0;
			STATE0 = vsha256hq_u32(STATE0, STATE1, TMP1);
			STATE1 = vsha256h2q_u32(STATE1, TMP2, TMP1);

			/* Combine state */
			STATE0 = vaddq_u32(STATE0, ABEF_SAVE);
			STATE1 = vaddq_u32(STATE1, CDGH_SAVE);

			/* Save state */
			vst1q_u32(&state[0], STATE0);
			vst1q_u32(&state[4], STATE1);
		}
#endif
	}
}
