#include "sha384.hpp"

#include <cstring> // memcpy

#include "sha512.hpp"
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
	std::string sha384::hash(const void* data, size_t len) SOUP_EXCAL
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
			sha512::processBlock(buffer, h);
		}

		StringWriter sw;
		sw.data.reserve(6 * 8);
		sw.u64_be(h[0]);
		sw.u64_be(h[1]);
		sw.u64_be(h[2]);
		sw.u64_be(h[3]);
		sw.u64_be(h[4]);
		sw.u64_be(h[5]);
		return std::move(sw.data);
	}

	std::string sha384::hash(const std::string& str) throw()
	{
		return hash(str.data(), str.size());
	}
}
