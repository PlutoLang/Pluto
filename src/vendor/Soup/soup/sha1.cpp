#include "sha1.hpp"

#include "base.hpp"

#if defined(SOUP_USE_INTRIN) && SOUP_BITS == 64 && (SOUP_X86 || SOUP_ARM)
#define SHA1_USE_INTRIN true
#else
#define SHA1_USE_INTRIN false
#endif

#if SHA1_USE_INTRIN
#include "CpuInfo.hpp"
#include "Endian.hpp"
#endif
#include "StringRefReader.hpp"

NAMESPACE_SOUP
{
#if SHA1_USE_INTRIN
	namespace intrin
	{
		extern void sha1_transform(uint32_t state[5], const uint8_t data[64]) noexcept;
	}
#endif

	// Original source: https://github.com/vog/sha1
	// Original licence: Dedicated to the public domain.

	template <size_t BLOCK_INTS, bool lsb>
	void buffer_to_block(const uint8_t* buffer, uint32_t block[BLOCK_INTS]) noexcept
	{
		/* Convert the std::string (byte buffer) to a uint32_t array */
		for (size_t i = 0; i < BLOCK_INTS; i++)
		{
			if constexpr (lsb)
			{
				block[i] = (buffer[4 * i + 0] & 0xff)
					| (buffer[4 * i + 1] & 0xff) << 8
					| (buffer[4 * i + 2] & 0xff) << 16
					| (buffer[4 * i + 3] & 0xff) << 24;
			}
			else
			{
				block[i] = (buffer[4 * i + 3] & 0xff)
					| (buffer[4 * i + 2] & 0xff) << 8
					| (buffer[4 * i + 1] & 0xff) << 16
					| (buffer[4 * i + 0] & 0xff) << 24;
			}
		}
	}

	static constexpr auto BLOCK_INTS = sha1::BLOCK_BYTES / sizeof(uint32_t);

	inline static uint32_t rol(const uint32_t value, const size_t bits) noexcept
	{
		return (value << bits) | (value >> (32 - bits));
	}

	inline static uint32_t blk(const uint32_t block[BLOCK_INTS], const size_t i) noexcept
	{
		return rol(block[(i + 13) & 15] ^ block[(i + 8) & 15] ^ block[(i + 2) & 15] ^ block[i], 1);
	}

	/*
	 * (R0+R1), R2, R3, R4 are the different operations used in SHA1
	 */

	inline static void R0(const uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i) noexcept
	{
		z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
		w = rol(w, 30);
	}

	inline static void R1(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i) noexcept
	{
		block[i] = blk(block, i);
		z += ((w & (x ^ y)) ^ y) + block[i] + 0x5a827999 + rol(v, 5);
		w = rol(w, 30);
	}

	inline static void R2(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i) noexcept
	{
		block[i] = blk(block, i);
		z += (w ^ x ^ y) + block[i] + 0x6ed9eba1 + rol(v, 5);
		w = rol(w, 30);
	}

	inline static void R3(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i) noexcept
	{
		block[i] = blk(block, i);
		z += (((w | x) & y) | (w & x)) + block[i] + 0x8f1bbcdc + rol(v, 5);
		w = rol(w, 30);
	}

	inline static void R4(uint32_t block[BLOCK_INTS], const uint32_t v, uint32_t& w, const uint32_t x, const uint32_t y, uint32_t& z, const size_t i) noexcept
	{
		block[i] = blk(block, i);
		z += (w ^ x ^ y) + block[i] + 0xca62c1d6 + rol(v, 5);
		w = rol(w, 30);
	}

	/*
	 * Hash a single 512-bit block. This is the core of the algorithm.
	 */

	inline static void transform_impl(uint32_t digest[], uint32_t block[BLOCK_INTS]) noexcept
	{
		/* Copy digest[] to working vars */
		uint32_t a = digest[0];
		uint32_t b = digest[1];
		uint32_t c = digest[2];
		uint32_t d = digest[3];
		uint32_t e = digest[4];

		/* 4 rounds of 20 operations each. Loop unrolled. */
		R0(block, a, b, c, d, e, 0);
		R0(block, e, a, b, c, d, 1);
		R0(block, d, e, a, b, c, 2);
		R0(block, c, d, e, a, b, 3);
		R0(block, b, c, d, e, a, 4);
		R0(block, a, b, c, d, e, 5);
		R0(block, e, a, b, c, d, 6);
		R0(block, d, e, a, b, c, 7);
		R0(block, c, d, e, a, b, 8);
		R0(block, b, c, d, e, a, 9);
		R0(block, a, b, c, d, e, 10);
		R0(block, e, a, b, c, d, 11);
		R0(block, d, e, a, b, c, 12);
		R0(block, c, d, e, a, b, 13);
		R0(block, b, c, d, e, a, 14);
		R0(block, a, b, c, d, e, 15);
		R1(block, e, a, b, c, d, 0);
		R1(block, d, e, a, b, c, 1);
		R1(block, c, d, e, a, b, 2);
		R1(block, b, c, d, e, a, 3);
		R2(block, a, b, c, d, e, 4);
		R2(block, e, a, b, c, d, 5);
		R2(block, d, e, a, b, c, 6);
		R2(block, c, d, e, a, b, 7);
		R2(block, b, c, d, e, a, 8);
		R2(block, a, b, c, d, e, 9);
		R2(block, e, a, b, c, d, 10);
		R2(block, d, e, a, b, c, 11);
		R2(block, c, d, e, a, b, 12);
		R2(block, b, c, d, e, a, 13);
		R2(block, a, b, c, d, e, 14);
		R2(block, e, a, b, c, d, 15);
		R2(block, d, e, a, b, c, 0);
		R2(block, c, d, e, a, b, 1);
		R2(block, b, c, d, e, a, 2);
		R2(block, a, b, c, d, e, 3);
		R2(block, e, a, b, c, d, 4);
		R2(block, d, e, a, b, c, 5);
		R2(block, c, d, e, a, b, 6);
		R2(block, b, c, d, e, a, 7);
		R3(block, a, b, c, d, e, 8);
		R3(block, e, a, b, c, d, 9);
		R3(block, d, e, a, b, c, 10);
		R3(block, c, d, e, a, b, 11);
		R3(block, b, c, d, e, a, 12);
		R3(block, a, b, c, d, e, 13);
		R3(block, e, a, b, c, d, 14);
		R3(block, d, e, a, b, c, 15);
		R3(block, c, d, e, a, b, 0);
		R3(block, b, c, d, e, a, 1);
		R3(block, a, b, c, d, e, 2);
		R3(block, e, a, b, c, d, 3);
		R3(block, d, e, a, b, c, 4);
		R3(block, c, d, e, a, b, 5);
		R3(block, b, c, d, e, a, 6);
		R3(block, a, b, c, d, e, 7);
		R3(block, e, a, b, c, d, 8);
		R3(block, d, e, a, b, c, 9);
		R3(block, c, d, e, a, b, 10);
		R3(block, b, c, d, e, a, 11);
		R4(block, a, b, c, d, e, 12);
		R4(block, e, a, b, c, d, 13);
		R4(block, d, e, a, b, c, 14);
		R4(block, c, d, e, a, b, 15);
		R4(block, b, c, d, e, a, 0);
		R4(block, a, b, c, d, e, 1);
		R4(block, e, a, b, c, d, 2);
		R4(block, d, e, a, b, c, 3);
		R4(block, c, d, e, a, b, 4);
		R4(block, b, c, d, e, a, 5);
		R4(block, a, b, c, d, e, 6);
		R4(block, e, a, b, c, d, 7);
		R4(block, d, e, a, b, c, 8);
		R4(block, c, d, e, a, b, 9);
		R4(block, b, c, d, e, a, 10);
		R4(block, a, b, c, d, e, 11);
		R4(block, e, a, b, c, d, 12);
		R4(block, d, e, a, b, c, 13);
		R4(block, c, d, e, a, b, 14);
		R4(block, b, c, d, e, a, 15);

		/* Add the working vars back into digest[] */
		digest[0] += a;
		digest[1] += b;
		digest[2] += c;
		digest[3] += d;
		digest[4] += e;
	}

	std::string sha1::hash(const void* data, size_t len) SOUP_EXCAL
	{
		State sha;
		sha.append(data, len);
		sha.finalise();
		return sha.getDigest();
	}

	std::string sha1::hash(Reader& r) SOUP_EXCAL
	{
		State sha;
		while (r.hasMore())
		{
			uint8_t b;
			r.u8(b);
			sha.appendByte(b);
		}
		sha.finalise();
		return sha.getDigest();
	}

#if SHA1_USE_INTRIN
	[[nodiscard]] static bool sha1_can_use_intrin() noexcept
	{
	#if SOUP_X86
		const CpuInfo& cpu_info = CpuInfo::get();
		return cpu_info.supportsSSSE3()
			&& cpu_info.supportsSHA()
			;
	#elif SOUP_ARM
		return CpuInfo::get().armv8_sha1;
	#endif
	}
#endif

	sha1::State::State()
	{
		state[0] = 0x67452301;
		state[1] = 0xefcdab89;
		state[2] = 0x98badcfe;
		state[3] = 0x10325476;
		state[4] = 0xc3d2e1f0;
		buffer_counter = 0;
		n_bits = 0;
	}

	void sha1::State::transform() noexcept
	{
#if SHA1_USE_INTRIN
		static bool good_cpu = sha1_can_use_intrin();
		if (good_cpu)
		{
	#if SOUP_X86
			intrin::sha1_transform(state, buffer);
	#else
			uint32_t block[BLOCK_INTS];
			buffer_to_block<BLOCK_INTS, false>(buffer, block);
			intrin::sha1_transform(state, (const uint8_t*)block);
	#endif
			return;
		}
#endif
		uint32_t block[BLOCK_INTS];
		buffer_to_block<BLOCK_INTS, false>(buffer, block);
		transform_impl(state, block);
	}

	void sha1::State::finalise() noexcept
	{
		uint64_t n_bits = this->n_bits;

		appendByte(0x80);

		while (buffer_counter != 56)
		{
			appendByte(0);
		}

		for (int i = 7; i >= 0; i--)
		{
			appendByte((n_bits >> 8 * i) & 0xff);
		}
	}

	void sha1::State::getDigest(uint8_t out[DIGEST_BYTES]) const noexcept
	{
		for (unsigned int i = 0; i != DIGEST_BYTES / 4; i++)
		{
			for (int j = 3; j >= 0; j--)
			{
				*out++ = (state[i] >> j * 8) & 0xff;
			}
		}
	}

	std::string sha1::State::getDigest() const SOUP_EXCAL
	{
		std::string digest(DIGEST_BYTES, '\0');
		getDigest((uint8_t*)digest.data());
		return digest;
	}
}
