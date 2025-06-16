// THIS FILE IS FOR INTERNAL USE ONLY. DO NOT INCLUDE THIS IN YOUR OWN CODE.

#include "base.hpp"

#include <cstdint>

#if SOUP_X86
	#include <wmmintrin.h>
#elif SOUP_ARM
	#include <arm_neon.h>
#endif

// x86:
// - https://gist.github.com/acapola/d5b940da024080dfaf5f
// - https://www.intel.com/content/dam/doc/white-paper/advanced-encryption-standard-new-instructions-set-paper.pdf
// ARM:
// - https://blog.michaelbrase.com/2018/06/04/optimizing-x86-aes-intrinsics-on-armv8-a/

NAMESPACE_SOUP
{
	namespace intrin
	{
#if SOUP_X86
		[[nodiscard]] static __m128i aes_expand_key_step(__m128i key0, __m128i key1) noexcept
		{
			key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
			key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
			key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
			return _mm_xor_si128(key0, _mm_shuffle_epi32(key1, 0xff));
		}

		[[nodiscard]]
	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		static __m128i aes_expand_key_odd_step(__m128i key0, __m128i key1) noexcept
		{
			key0 = _mm_aeskeygenassist_si128(key0, 0);
			key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
			key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
			key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
			return _mm_xor_si128(key1, _mm_shuffle_epi32(key0, 0xaa));
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_expand_key_128(uint8_t w[176], const uint8_t key[16]) noexcept
		{
			reinterpret_cast<__m128i*>(w)[0] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(key));
			reinterpret_cast<__m128i*>(w)[1] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[0], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[0], 0x01));
			reinterpret_cast<__m128i*>(w)[2] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[1], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[1], 0x02));
			reinterpret_cast<__m128i*>(w)[3] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[2], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[2], 0x04));
			reinterpret_cast<__m128i*>(w)[4] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[3], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[3], 0x08));
			reinterpret_cast<__m128i*>(w)[5] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[4], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[4], 0x10));
			reinterpret_cast<__m128i*>(w)[6] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[5], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[5], 0x20));
			reinterpret_cast<__m128i*>(w)[7] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[6], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[6], 0x40));
			reinterpret_cast<__m128i*>(w)[8] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[7], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[7], 0x80));
			reinterpret_cast<__m128i*>(w)[9] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[8], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[8], 0x1B));
			reinterpret_cast<__m128i*>(w)[10] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[9], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[9], 0x36));
		}

		static void KEY_192_ASSIST(__m128i* temp1, __m128i* temp2, __m128i* temp3) noexcept
		{
			__m128i temp4;
			*temp2 = _mm_shuffle_epi32(*temp2, 0x55);
			temp4 = _mm_slli_si128(*temp1, 0x4);
			*temp1 = _mm_xor_si128(*temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			*temp1 = _mm_xor_si128(*temp1, temp4);
			temp4 = _mm_slli_si128(temp4, 0x4);
			*temp1 = _mm_xor_si128(*temp1, temp4);
			*temp1 = _mm_xor_si128(*temp1, *temp2);
			*temp2 = _mm_shuffle_epi32(*temp1, 0xff);
			temp4 = _mm_slli_si128(*temp3, 0x4);
			*temp3 = _mm_xor_si128(*temp3, temp4);
			*temp3 = _mm_xor_si128(*temp3, *temp2);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_expand_key_192(uint8_t w[208], const uint8_t key[24]) noexcept
		{
			__m128i temp1, temp2, temp3;
			__m128i* Key_Schedule = (__m128i*)w;
			temp1 = _mm_loadu_si128((__m128i*)key);
			temp3 = _mm_set_epi64x(0, *reinterpret_cast<const uint64_t*>(key + 16));
			Key_Schedule[0] = temp1;
			Key_Schedule[1] = temp3;
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x1);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[1] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Key_Schedule[1]), _mm_castsi128_pd(temp1), 0));
			Key_Schedule[2] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(temp1), _mm_castsi128_pd(temp3), 1));
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x2);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[3] = temp1;
			Key_Schedule[4] = temp3;
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x4);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[4] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Key_Schedule[4]), _mm_castsi128_pd(temp1), 0));
			Key_Schedule[5] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(temp1), _mm_castsi128_pd(temp3), 1));
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x8);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[6] = temp1;
			Key_Schedule[7] = temp3;
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x10);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[7] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Key_Schedule[7]), _mm_castsi128_pd(temp1), 0));
			Key_Schedule[8] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(temp1), _mm_castsi128_pd(temp3), 1));
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x20);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[9] = temp1;
			Key_Schedule[10] = temp3;
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x40);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[10] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(Key_Schedule[10]), _mm_castsi128_pd(temp1), 0));
			Key_Schedule[11] = _mm_castpd_si128(_mm_shuffle_pd(_mm_castsi128_pd(temp1), _mm_castsi128_pd(temp3), 1));
			temp2 = _mm_aeskeygenassist_si128(temp3, 0x80);
			KEY_192_ASSIST(&temp1, &temp2, &temp3);
			Key_Schedule[12] = temp1;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_expand_key_256(uint8_t w[240], const uint8_t key[32]) noexcept
		{
			reinterpret_cast<__m128i*>(w)[0] = _mm_loadu_si128(&reinterpret_cast<const __m128i*>(key)[0]);
			reinterpret_cast<__m128i*>(w)[1] = _mm_loadu_si128(&reinterpret_cast<const __m128i*>(key)[1]);
			reinterpret_cast<__m128i*>(w)[2] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[0], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[1], 0x01));
			reinterpret_cast<__m128i*>(w)[3] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[2], reinterpret_cast<__m128i*>(w)[1]);
			reinterpret_cast<__m128i*>(w)[4] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[2], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[3], 0x02));
			reinterpret_cast<__m128i*>(w)[5] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[4], reinterpret_cast<__m128i*>(w)[3]);
			reinterpret_cast<__m128i*>(w)[6] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[4], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[5], 0x04));
			reinterpret_cast<__m128i*>(w)[7] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[6], reinterpret_cast<__m128i*>(w)[5]);
			reinterpret_cast<__m128i*>(w)[8] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[6], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[7], 0x08));
			reinterpret_cast<__m128i*>(w)[9] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[8], reinterpret_cast<__m128i*>(w)[7]);
			reinterpret_cast<__m128i*>(w)[10] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[8], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[9], 0x10));
			reinterpret_cast<__m128i*>(w)[11] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[10], reinterpret_cast<__m128i*>(w)[9]);
			reinterpret_cast<__m128i*>(w)[12] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[10], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[11], 0x20));
			reinterpret_cast<__m128i*>(w)[13] = aes_expand_key_odd_step(reinterpret_cast<const __m128i*>(w)[12], reinterpret_cast<__m128i*>(w)[11]);
			reinterpret_cast<__m128i*>(w)[14] = aes_expand_key_step(reinterpret_cast<const __m128i*>(w)[12], _mm_aeskeygenassist_si128(reinterpret_cast<const __m128i*>(w)[13], 0x40));
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_encrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesenclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			*reinterpret_cast<__m128i*>(out) = data;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_encrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[11]);
			data = _mm_aesenclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[12]);
			*reinterpret_cast<__m128i*>(out) = data;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_encrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[11]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[12]);
			data = _mm_aesenc_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[13]);
			data = _mm_aesenclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[14]);
			*reinterpret_cast<__m128i*>(out) = data;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_prepare_decryption_128(uint8_t w[176]) noexcept
		{
			reinterpret_cast<__m128i*>(w)[1] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[1]);
			reinterpret_cast<__m128i*>(w)[2] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[2]);
			reinterpret_cast<__m128i*>(w)[3] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[3]);
			reinterpret_cast<__m128i*>(w)[4] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[4]);
			reinterpret_cast<__m128i*>(w)[5] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[5]);
			reinterpret_cast<__m128i*>(w)[6] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[6]);
			reinterpret_cast<__m128i*>(w)[7] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[7]);
			reinterpret_cast<__m128i*>(w)[8] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[8]);
			reinterpret_cast<__m128i*>(w)[9] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[9]);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_prepare_decryption_192(uint8_t w[208]) noexcept
		{
			reinterpret_cast<__m128i*>(w)[1] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[1]);
			reinterpret_cast<__m128i*>(w)[2] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[2]);
			reinterpret_cast<__m128i*>(w)[3] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[3]);
			reinterpret_cast<__m128i*>(w)[4] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[4]);
			reinterpret_cast<__m128i*>(w)[5] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[5]);
			reinterpret_cast<__m128i*>(w)[6] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[6]);
			reinterpret_cast<__m128i*>(w)[7] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[7]);
			reinterpret_cast<__m128i*>(w)[8] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[8]);
			reinterpret_cast<__m128i*>(w)[9] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[9]);
			reinterpret_cast<__m128i*>(w)[10] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[10]);
			reinterpret_cast<__m128i*>(w)[11] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[11]);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_prepare_decryption_256(uint8_t w[240]) noexcept
		{
			reinterpret_cast<__m128i*>(w)[1] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[1]);
			reinterpret_cast<__m128i*>(w)[2] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[2]);
			reinterpret_cast<__m128i*>(w)[3] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[3]);
			reinterpret_cast<__m128i*>(w)[4] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[4]);
			reinterpret_cast<__m128i*>(w)[5] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[5]);
			reinterpret_cast<__m128i*>(w)[6] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[6]);
			reinterpret_cast<__m128i*>(w)[7] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[7]);
			reinterpret_cast<__m128i*>(w)[8] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[8]);
			reinterpret_cast<__m128i*>(w)[9] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[9]);
			reinterpret_cast<__m128i*>(w)[10] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[10]);
			reinterpret_cast<__m128i*>(w)[11] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[11]);
			reinterpret_cast<__m128i*>(w)[12] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[12]);
			reinterpret_cast<__m128i*>(w)[13] = _mm_aesimc_si128(reinterpret_cast<__m128i*>(w)[13]);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_decrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesdeclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			*reinterpret_cast<__m128i*>(out) = data;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_decrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[12]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[11]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesdeclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			*reinterpret_cast<__m128i*>(out) = data;
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("aes")))
	#endif
		void aes_decrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
		{
			__m128i data = *reinterpret_cast<const __m128i*>(in);
			data = _mm_xor_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[14]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[13]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[12]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[11]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[10]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[9]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[8]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[7]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[6]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[5]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[4]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[3]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[2]);
			data = _mm_aesdec_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[1]);
			data = _mm_aesdeclast_si128(data, reinterpret_cast<const __m128i*>(roundKeys)[0]);
			*reinterpret_cast<__m128i*>(out) = data;
		}
#elif SOUP_ARM
	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_encrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaeseq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[1 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[2 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[3 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[4 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[5 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[6 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[7 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[8 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[9 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[10 * 16]));
			vst1q_u8(out, data);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_encrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaeseq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[1 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[2 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[3 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[4 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[5 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[6 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[7 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[8 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[9 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[10 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[11 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[12 * 16]));
			vst1q_u8(out, data);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_encrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaeseq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[1 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[2 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[3 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[4 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[5 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[6 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[7 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[8 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[9 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[10 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[11 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[12 * 16]));
			data = vaeseq_u8(vaesmcq_u8(data), vld1q_u8(&roundKeys[13 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[14 * 16]));
			vst1q_u8(out, data);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_prepare_decryption_128(uint8_t w[176]) noexcept
		{
			vst1q_u8(&w[1 * 16], vaesimcq_u8(vld1q_u8(&w[1 * 16])));
			vst1q_u8(&w[2 * 16], vaesimcq_u8(vld1q_u8(&w[2 * 16])));
			vst1q_u8(&w[3 * 16], vaesimcq_u8(vld1q_u8(&w[3 * 16])));
			vst1q_u8(&w[4 * 16], vaesimcq_u8(vld1q_u8(&w[4 * 16])));
			vst1q_u8(&w[5 * 16], vaesimcq_u8(vld1q_u8(&w[5 * 16])));
			vst1q_u8(&w[6 * 16], vaesimcq_u8(vld1q_u8(&w[6 * 16])));
			vst1q_u8(&w[7 * 16], vaesimcq_u8(vld1q_u8(&w[7 * 16])));
			vst1q_u8(&w[8 * 16], vaesimcq_u8(vld1q_u8(&w[8 * 16])));
			vst1q_u8(&w[9 * 16], vaesimcq_u8(vld1q_u8(&w[9 * 16])));
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_prepare_decryption_192(uint8_t w[208]) noexcept
		{
			vst1q_u8(&w[1 * 16], vaesimcq_u8(vld1q_u8(&w[1 * 16])));
			vst1q_u8(&w[2 * 16], vaesimcq_u8(vld1q_u8(&w[2 * 16])));
			vst1q_u8(&w[3 * 16], vaesimcq_u8(vld1q_u8(&w[3 * 16])));
			vst1q_u8(&w[4 * 16], vaesimcq_u8(vld1q_u8(&w[4 * 16])));
			vst1q_u8(&w[5 * 16], vaesimcq_u8(vld1q_u8(&w[5 * 16])));
			vst1q_u8(&w[6 * 16], vaesimcq_u8(vld1q_u8(&w[6 * 16])));
			vst1q_u8(&w[7 * 16], vaesimcq_u8(vld1q_u8(&w[7 * 16])));
			vst1q_u8(&w[8 * 16], vaesimcq_u8(vld1q_u8(&w[8 * 16])));
			vst1q_u8(&w[9 * 16], vaesimcq_u8(vld1q_u8(&w[9 * 16])));
			vst1q_u8(&w[10 * 16], vaesimcq_u8(vld1q_u8(&w[10 * 16])));
			vst1q_u8(&w[11 * 16], vaesimcq_u8(vld1q_u8(&w[11 * 16])));
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_prepare_decryption_256(uint8_t w[240]) noexcept
		{
			vst1q_u8(&w[1 * 16], vaesimcq_u8(vld1q_u8(&w[1 * 16])));
			vst1q_u8(&w[2 * 16], vaesimcq_u8(vld1q_u8(&w[2 * 16])));
			vst1q_u8(&w[3 * 16], vaesimcq_u8(vld1q_u8(&w[3 * 16])));
			vst1q_u8(&w[4 * 16], vaesimcq_u8(vld1q_u8(&w[4 * 16])));
			vst1q_u8(&w[5 * 16], vaesimcq_u8(vld1q_u8(&w[5 * 16])));
			vst1q_u8(&w[6 * 16], vaesimcq_u8(vld1q_u8(&w[6 * 16])));
			vst1q_u8(&w[7 * 16], vaesimcq_u8(vld1q_u8(&w[7 * 16])));
			vst1q_u8(&w[8 * 16], vaesimcq_u8(vld1q_u8(&w[8 * 16])));
			vst1q_u8(&w[9 * 16], vaesimcq_u8(vld1q_u8(&w[9 * 16])));
			vst1q_u8(&w[10 * 16], vaesimcq_u8(vld1q_u8(&w[10 * 16])));
			vst1q_u8(&w[11 * 16], vaesimcq_u8(vld1q_u8(&w[11 * 16])));
			vst1q_u8(&w[12 * 16], vaesimcq_u8(vld1q_u8(&w[12 * 16])));
			vst1q_u8(&w[13 * 16], vaesimcq_u8(vld1q_u8(&w[13 * 16])));
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_decrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[10 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[9 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[8 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[7 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[6 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[5 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[4 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[3 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[2 * 16])));
			data = vaesdq_u8(data, vld1q_u8(&roundKeys[1 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			vst1q_u8(out, data);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_decrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[12 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[11 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[10 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[9 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[8 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[7 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[6 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[5 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[4 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[3 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[2 * 16])));
			data = vaesdq_u8(data, vld1q_u8(&roundKeys[1 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			vst1q_u8(out, data);
		}

	#if defined(__GNUC__) || defined(__clang__)
		__attribute__((target("arch=armv8-a+crypto")))
	#endif
		void aes_decrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
		{
			auto data = vld1q_u8(in);
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[14 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[13 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[12 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[11 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[10 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[9 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[8 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[7 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[6 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[5 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[4 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[3 * 16])));
			data = vaesimcq_u8(vaesdq_u8(data, vld1q_u8(&roundKeys[2 * 16])));
			data = vaesdq_u8(data, vld1q_u8(&roundKeys[1 * 16]));
			data = veorq_u8(data, vld1q_u8(&roundKeys[0 * 16]));
			vst1q_u8(out, data);
		}
#endif
	}
}
