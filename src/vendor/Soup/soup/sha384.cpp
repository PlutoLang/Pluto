#include "sha384.hpp"

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
		State sha;
		sha.append(data, len);
		sha.finalise();
		return sha.getDigest();
	}

	sha384::State::State() noexcept
	{
		// implicitly calls sha512::State::State()
		state[0] = 0xcbbb9d5dc1059ed8ULL;
		state[1] = 0x629a292a367cd507ULL;
		state[2] = 0x9159015a3070dd17ULL;
		state[3] = 0x152fecd8f70e5939ULL;
		state[4] = 0x67332667ffc00b31ULL;
		state[5] = 0x8eb44a8768581511ULL;
		state[6] = 0xdb0c2e0d64f98fa7ULL;
		state[7] = 0x47b5481dbefa4fa4ULL;
		//buffer_counter = 0;
		//n_bits = 0;
	}

	void sha384::State::getDigest(uint8_t out[DIGEST_BYTES]) const noexcept
	{
		for (unsigned int i = 0; i != DIGEST_BYTES / 8; i++)
		{
			for (int j = 7; j >= 0; j--)
			{
				*out++ = (state[i] >> j * 8) & 0xff;
			}
		}
	}

	std::string sha384::State::getDigest() const SOUP_EXCAL
	{
		std::string digest(DIGEST_BYTES, '\0');
		getDigest((uint8_t*)digest.data());
		return digest;
	}
}
