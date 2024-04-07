#include "sha512.hpp"

#include <cstring> // memcpy

#include "StringWriter.hpp"

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
		uint64_t h[HASH_LEN]; // buffer holding the message digest (512-bit = 8 64-bit words)
		memcpy(h, hPrime, WORKING_VAR_LEN * sizeof(uint64_t));

		const size_t l = len * CHAR_LEN_BITS; // length of input in bits
		const size_t k = (896 - 1 - l) % MESSAGE_BLOCK_SIZE; // length of zero bit padding (l + 1 + k = 896 mod 1024) 
		const size_t nBuffer = (l + 1 + k + 128) / MESSAGE_BLOCK_SIZE;

		for (size_t i = 0; i != nBuffer; ++i)
		{
			uint64_t buffer[SEQUENCE_LEN];
			for (size_t j = 0; j != SEQUENCE_LEN; ++j)
			{
				uint64_t in = 0x0ULL;
				for (size_t k = 0; k != WORD_LEN; ++k)
				{
					size_t index = i * 128 + j * 8 + k;
					if (index < len)
					{
						in = in << 8 | (uint64_t)reinterpret_cast<const uint8_t*>(data)[index];
					}
					else if (index == len)
					{
						in = in << 8 | 0x80ULL;
					}
					else
					{
						in = in << 8 | 0x0ULL;
					}
				}
				buffer[j] = in;
			}
			if (i == nBuffer - 1)
			{
				buffer[SEQUENCE_LEN - 1] = l;
				buffer[SEQUENCE_LEN - 2] = 0x00ULL;
			}
			processBlock(buffer, h);
		}

		StringWriter sw;
		sw.data.reserve(8 * 8);
		sw.u64_be(h[0]);
		sw.u64_be(h[1]);
		sw.u64_be(h[2]);
		sw.u64_be(h[3]);
		sw.u64_be(h[4]);
		sw.u64_be(h[5]);
		sw.u64_be(h[6]);
		sw.u64_be(h[7]);
		return std::move(sw.data);
	}

	std::string sha512::hash(const std::string& str) SOUP_EXCAL
	{
		return hash(str.data(), str.size());
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

#define Ch(x,y,z) ((x&y)^(~x&z))
#define Maj(x,y,z) ((x&y)^(x&z)^(y&z))
#define RotR(x, n) ((x>>n)|(x<<((sizeof(x)<<3)-n)))
#define Sig0(x) ((RotR(x, 28))^(RotR(x,34))^(RotR(x, 39)))
#define Sig1(x) ((RotR(x, 14))^(RotR(x,18))^(RotR(x, 41)))
#define sig0(x) (RotR(x, 1)^RotR(x,8)^(x>>7))
#define sig1(x) (RotR(x, 19)^RotR(x,61)^(x>>6))

	void sha512::processBlock(uint64_t block[16], uint64_t h[8])
	{
		uint64_t s[WORKING_VAR_LEN];
		uint64_t w[MESSAGE_SCHEDULE_LEN];

		// copy over to message schedule
		memcpy(w, block, SEQUENCE_LEN * sizeof(uint64_t));

		// Prepare the message schedule
		for (size_t j = 16; j != MESSAGE_SCHEDULE_LEN; ++j)
		{
			w[j] = w[j - 16] + sig0(w[j - 15]) + w[j - 7] + sig1(w[j - 2]);
		}
		// Initialize the working variables
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
}
