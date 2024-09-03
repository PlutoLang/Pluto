#include "sha512.hpp"

#include <cstring> // memcpy

#include "Endian.hpp"

/*
Original source: https://github.com/pr0f3ss/SHA
Original licence follows.

MIT License

Copyright (c) 2022 Filip Dobrosavljevic

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

NAMESPACE_SOUP
{
	std::string sha512::hash(const void* data, size_t len) SOUP_EXCAL
	{
		State sha;
		sha.append(data, len);
		sha.finalise();
		return sha.getDigest();
	}

	static const uint64_t k[80] = { 0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL, 0x3956c25bf348b538ULL,
		  0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL, 0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
		  0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL, 0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL,
		  0xc19bf174cf692694ULL, 0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
		  0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL, 0x983e5152ee66dfabULL,
		  0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL, 0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
		  0x06ca6351e003826fULL, 0x142929670a0e6e70ULL, 0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL,
		  0x53380d139d95b3dfULL, 0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
		  0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL, 0xd192e819d6ef5218ULL,
		  0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL, 0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
		  0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL, 0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL,
		  0x682e6ff3d6b2b8a3ULL, 0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
		  0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL, 0xca273eceea26619cULL,
		  0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL, 0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
		  0x113f9804bef90daeULL, 0x1b710b35131c471bULL, 0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL,
		  0x431d67c49c100d4cULL, 0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL };

	static constexpr size_t MESSAGE_SCHEDULE_LEN = 80;
	static constexpr size_t WORKING_VAR_LEN = 8;

#define Ch(x,y,z) ((x&y)^(~x&z))
#define Maj(x,y,z) ((x&y)^(x&z)^(y&z))
#define RotR(x, n) ((x>>n)|(x<<((sizeof(x)<<3)-n)))
#define Sig0(x) ((RotR(x, 28))^(RotR(x,34))^(RotR(x, 39)))
#define Sig1(x) ((RotR(x, 14))^(RotR(x,18))^(RotR(x, 41)))
#define sig0(x) (RotR(x, 1)^RotR(x,8)^(x>>7))
#define sig1(x) (RotR(x, 19)^RotR(x,61)^(x>>6))

	sha512::State::State() noexcept
	{
		state[0] = 0x6a09e667f3bcc908ULL;
		state[1] = 0xbb67ae8584caa73bULL;
		state[2] = 0x3c6ef372fe94f82bULL;
		state[3] = 0xa54ff53a5f1d36f1ULL;
		state[4] = 0x510e527fade682d1ULL;
		state[5] = 0x9b05688c2b3e6c1fULL;
		state[6] = 0x1f83d9abfb41bd6bULL;
		state[7] = 0x5be0cd19137e2179ULL;
		buffer_counter = 0;
		n_bits = 0;
	}

	void sha512::State::transform() noexcept
	{
		// Initialise message schedule
		uint64_t w[MESSAGE_SCHEDULE_LEN];
		for (size_t i = 0; i != BLOCK_BYTES / 8; ++i)
		{
			w[i] = Endianness::invert(reinterpret_cast<const uint64_t*>(buffer)[i]);
		}
		for (size_t j = 16; j != MESSAGE_SCHEDULE_LEN; ++j)
		{
			w[j] = w[j - 16] + sig0(w[j - 15]) + w[j - 7] + sig1(w[j - 2]);
		}

		// Initialise the working variables
		uint64_t* const h = this->state;
		uint64_t s[WORKING_VAR_LEN];
		memcpy(s, h, WORKING_VAR_LEN * sizeof(uint64_t));

		// Compression
		for (size_t j = 0; j != MESSAGE_SCHEDULE_LEN; ++j)
		{
			uint64_t temp1 = s[7] + Sig1(s[4]) + Ch(s[4], s[5], s[6]) + k[j] + w[j];
			uint64_t temp2 = Sig0(s[0]) + Maj(s[0], s[1], s[2]);

			s[7] = s[6];
			s[6] = s[5];
			s[5] = s[4];
			s[4] = s[3] + temp1;
			s[3] = s[2];
			s[2] = s[1];
			s[1] = s[0];
			s[0] = temp1 + temp2;
		}

		// Compute the intermediate hash values
		for (size_t j = 0; j != WORKING_VAR_LEN; ++j)
		{
			h[j] += s[j];
		}
	}

	void sha512::State::finalise() noexcept
	{
		uint64_t n_bits = this->n_bits;

		appendByte(0x80);

		while (buffer_counter != 120)
		{
			appendByte(0);
		}

		for (int i = 7; i >= 0; i--)
		{
			appendByte((n_bits >> 8 * i) & 0xff);
		}
	}

	void sha512::State::getDigest(uint8_t out[DIGEST_BYTES]) const noexcept
	{
		for (unsigned int i = 0; i != DIGEST_BYTES / 8; i++)
		{
			for (int j = 7; j >= 0; j--)
			{
				*out++ = (state[i] >> j * 8) & 0xff;
			}
		}
	}

	std::string sha512::State::getDigest() const SOUP_EXCAL
	{
		std::string digest(DIGEST_BYTES, '\0');
		getDigest((uint8_t*)digest.data());
		return digest;
	}
}
