#include "sha256.hpp"

#include "base.hpp"

#if defined(SOUP_USE_INTRIN) && SOUP_BITS == 64 && (SOUP_X86 || SOUP_ARM)
#define SHA256_USE_INTRIN true
#else
#define SHA256_USE_INTRIN false
#endif

#if SHA256_USE_INTRIN
#include "CpuInfo.hpp"
#endif
#include "Reader.hpp"

/*
Original source: https://github.com/983/SHA-256
Original licence: Dedicated to the public domain.
*/

NAMESPACE_SOUP
{
#if SHA256_USE_INTRIN
	namespace intrin
	{
		extern void sha256_transform(uint32_t state[8], const uint8_t data[64]) noexcept;
	}
#endif

	struct sha256_state
	{
		uint32_t state[8];
		uint8_t buffer[64];
		uint64_t n_bits;
		uint8_t buffer_counter;
	};

	static inline uint32_t rotr(uint32_t x, int n) noexcept
	{
		return (x >> n) | (x << (32 - n));
	}

	static inline uint32_t step1(uint32_t e, uint32_t f, uint32_t g) noexcept
	{
		return (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ ((~e) & g));
	}

	static inline uint32_t step2(uint32_t a, uint32_t b, uint32_t c) noexcept
	{
		return (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
	}

	static inline void update_w(uint32_t* w, int i, const uint8_t* buffer) noexcept
	{
		int j;
		for (j = 0; j < 16; j++) {
			if (i < 16) {
				w[j] =
					((uint32_t)buffer[0] << 24) |
					((uint32_t)buffer[1] << 16) |
					((uint32_t)buffer[2] << 8) |
					((uint32_t)buffer[3]);
				buffer += 4;
			}
			else {
				uint32_t a = w[(j + 1) & 15];
				uint32_t b = w[(j + 14) & 15];
				uint32_t s0 = (rotr(a, 7) ^ rotr(a, 18) ^ (a >> 3));
				uint32_t s1 = (rotr(b, 17) ^ rotr(b, 19) ^ (b >> 10));
				w[j] += w[(j + 9) & 15] + s0 + s1;
			}
		}
	}

#if SHA256_USE_INTRIN
	[[nodiscard]] static bool sha256_can_use_intrin() noexcept
	{
	#if SOUP_X86
		const CpuInfo& cpu_info = CpuInfo::get();
		return cpu_info.supportsSSE4_1() // _mm_blend_epi16
			&& cpu_info.supportsSHA()
			;
	#elif SOUP_ARM
		return CpuInfo::get().armv8_sha2;
	#endif
	}
#endif

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

	std::string sha256::hash(const void* data, size_t len) SOUP_EXCAL
	{
		State sha;
		sha.append(data, len);
		sha.finalise();
		return sha.getDigest();
	}

	std::string sha256::hash(Reader& r) SOUP_EXCAL
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

	sha256::State::State() noexcept
	{
		state[0] = 0x6a09e667;
		state[1] = 0xbb67ae85;
		state[2] = 0x3c6ef372;
		state[3] = 0xa54ff53a;
		state[4] = 0x510e527f;
		state[5] = 0x9b05688c;
		state[6] = 0x1f83d9ab;
		state[7] = 0x5be0cd19;
		buffer_counter = 0;
		n_bits = 0;
	}

	void sha256::State::transform() noexcept
	{
#if SHA256_USE_INTRIN
		static bool good_cpu = sha256_can_use_intrin();
		if (good_cpu)
		{
			return intrin::sha256_transform(this->state, this->buffer);
		}
#endif

		uint32_t* state = this->state;

		uint32_t a = state[0];
		uint32_t b = state[1];
		uint32_t c = state[2];
		uint32_t d = state[3];
		uint32_t e = state[4];
		uint32_t f = state[5];
		uint32_t g = state[6];
		uint32_t h = state[7];

		uint32_t w[16];

		int i, j;
		for (i = 0; i < 64; i += 16) {
			update_w(w, i, buffer);

			for (j = 0; j < 16; j += 4) {
				uint32_t temp;
				temp = h + step1(e, f, g) + sha256_k[i + j + 0] + w[j + 0];
				h = temp + d;
				d = temp + step2(a, b, c);
				temp = g + step1(h, e, f) + sha256_k[i + j + 1] + w[j + 1];
				g = temp + c;
				c = temp + step2(d, a, b);
				temp = f + step1(g, h, e) + sha256_k[i + j + 2] + w[j + 2];
				f = temp + b;
				b = temp + step2(c, d, a);
				temp = e + step1(f, g, h) + sha256_k[i + j + 3] + w[j + 3];
				e = temp + a;
				a = temp + step2(b, c, d);
			}
		}

		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		state[5] += f;
		state[6] += g;
		state[7] += h;
	}

	void sha256::State::finalise() noexcept
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

	void sha256::State::getDigest(uint8_t out[DIGEST_BYTES]) const noexcept
	{
		for (unsigned int i = 0; i != DIGEST_BYTES / 4; i++)
		{
			for (int j = 3; j >= 0; j--)
			{
				*out++ = (state[i] >> j * 8) & 0xff;
			}
		}
	}

	std::string sha256::State::getDigest() const SOUP_EXCAL
	{
		std::string digest(DIGEST_BYTES, '\0');
		getDigest((uint8_t*)digest.data());
		return digest;
	}
}
