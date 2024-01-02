#if defined(__x86_64__) || defined(_M_X64)

#include <cstdint>

#include <wmmintrin.h>

namespace soup
{
	[[nodiscard]] static __m128i aes_expand_key_step(__m128i key0, __m128i key1) noexcept
	{
		key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
		key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
		key0 = _mm_xor_si128(key0, _mm_slli_si128(key0, 4));
		return _mm_xor_si128(key0, _mm_shuffle_epi32(key1, 0xff));
	}

	[[nodiscard]] static __m128i aes_expand_key_odd_step(__m128i key0, __m128i key1) noexcept
	{
		key0 = _mm_aeskeygenassist_si128(key0, 0);
		key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
		key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
		key1 = _mm_xor_si128(key1, _mm_slli_si128(key1, 4));
		return _mm_xor_si128(key1, _mm_shuffle_epi32(key0, 0xaa));
	}

	void aes_helper_expand_key_128(uint8_t w[176], const uint8_t key[16]) noexcept
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

	void aes_helper_expand_key_256(uint8_t w[240], const uint8_t key[32]) noexcept
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

	void aes_helper_encrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[0]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[1]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[2]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[3]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[4]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[5]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[6]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[7]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[8]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[9]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[10]);
	}

	void aes_helper_encrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[0]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[1]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[2]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[3]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[4]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[5]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[6]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[7]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[8]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[9]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[10]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[11]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[12]);
	}

	void aes_helper_encrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[0]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[1]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[2]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[3]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[4]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[5]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[6]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[7]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[8]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[9]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[10]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[11]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[12]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenc_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[13]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesenclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[14]);
	}

	void aes_helper_decrypt_block_128(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[176]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[10]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[9]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[8]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[7]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[6]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[5]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[4]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[3]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[2]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[1]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdeclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[0]);
	}

	void aes_helper_decrypt_block_192(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[208]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[12]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[11]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[10]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[9]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[8]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[7]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[6]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[5]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[4]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[3]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[2]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[1]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdeclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[0]);
	}

	void aes_helper_decrypt_block_256(const uint8_t in[16], uint8_t out[16], const uint8_t roundKeys[240]) noexcept
	{
		*reinterpret_cast<__m128i*>(out) = _mm_xor_si128(*reinterpret_cast<const __m128i*>(in), reinterpret_cast<const __m128i*>(roundKeys)[14]);
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[13]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[12]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[11]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[10]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[9]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[8]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[7]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[6]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[5]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[4]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[3]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[2]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdec_si128(*reinterpret_cast<const __m128i*>(out), _mm_aesimc_si128(reinterpret_cast<const __m128i*>(roundKeys)[1]));
		*reinterpret_cast<__m128i*>(out) = _mm_aesdeclast_si128(*reinterpret_cast<const __m128i*>(out), reinterpret_cast<const __m128i*>(roundKeys)[0]);
	}
}

#endif
