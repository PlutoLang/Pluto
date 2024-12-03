#include "base64.hpp"

#include <cstdint>

/*
Original source: https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594
Original licence follows.

Copyright (c) 2016 tomykaira

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

NAMESPACE_SOUP
{
	static constexpr char table_encode_base64[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};

	std::string base64::encode(const char* data, const bool pad) SOUP_EXCAL
	{
		return encode(data, strlen(data), pad);
	}

	std::string base64::encode(const std::string& data, const bool pad) SOUP_EXCAL
	{
		return encode(data.data(), data.size(), pad);
	}

	std::string base64::encode(const char* const data, const size_t size, const bool pad) SOUP_EXCAL
	{
		return encode(data, size, pad, table_encode_base64);
	}

	void base64::encode(char* out, const char* const data, const size_t size, const bool pad) noexcept
	{
		return encode(out, data, size, pad, table_encode_base64);
	}

	static constexpr char table_encode_base64url[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '-', '_'
	};

	std::string base64::urlEncode(const char* data, const bool pad) SOUP_EXCAL
	{
		return urlEncode(data, strlen(data), pad);
	}

	std::string base64::urlEncode(const std::string& data, const bool pad) SOUP_EXCAL
	{
		return urlEncode(data.data(), data.size(), pad);
	}

	std::string base64::urlEncode(const char* const data, const size_t size, const bool pad) SOUP_EXCAL
	{
		return encode(data, size, pad, table_encode_base64url);
	}

	void base64::urlEncode(char* out, const char* const data, const size_t size, const bool pad) noexcept
	{
		return encode(out, data, size, pad, table_encode_base64url);
	}

	std::string base64::encode(const char* const data, const size_t size, const bool pad, const char table[64]) SOUP_EXCAL
	{
		std::string enc(getEncodedSize(size, pad), '\0');
		encode(enc.data(), data, size, pad, table);
		return enc;
	}

	void base64::encode(char* out, const char* data, const size_t size, const bool pad, const char table[64]) noexcept
	{
		size_t i = 0;

		if (size > 2)
		{
			for (; i < size - 2; i += 3)
			{
				*out++ = table[(data[i] >> 2) & 0x3F];
				*out++ = table[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*out++ = table[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
				*out++ = table[data[i + 2] & 0x3F];
			}
		}

		if (i < size)
		{
			*out++ = table[(data[i] >> 2) & 0x3F];
			if (i == (size - 1))
			{
				*out++ = table[((data[i] & 0x3) << 4)];
				if (pad)
				{
					*out++ = '=';
				}
			}
			else
			{
				*out++ = table[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*out++ = table[((data[i + 1] & 0xF) << 2)];
			}
			if (pad)
			{
				*out++ = '=';
			}
		}
	}

	static constexpr unsigned char table_decode_base64[] = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
		64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	size_t base64::getDecodedSize(const char* data, size_t size) noexcept
	{
		// Ignore pad bytes
		while (size != 0 && data[size - 1] == '=')
		{
			size--;
		}

		size_t out_len = size / 4 * 3;
		if (auto remainder = (size % 4))
		{
			out_len += (remainder - 1) + (remainder == 1);
		}
		return out_len;
	}

	std::string base64::decode(const std::string& enc) SOUP_EXCAL
	{
		return decode(enc, table_decode_base64);
	}

	void base64::decode(char* out, const char* data, size_t size) noexcept
	{
		return decode(out, data, size, table_decode_base64);
	}

	static constexpr unsigned char table_decode_base64url[] = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64,
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
		64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
		64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	std::string base64::urlDecode(const std::string& enc) SOUP_EXCAL
	{
		return decode(enc, table_decode_base64url);
	}

	void base64::urlDecode(char* out, const char* data, size_t size) noexcept
	{
		return decode(out, data, size, table_decode_base64url);
	}

	std::string base64::decode(const std::string& enc, const unsigned char table[256]) SOUP_EXCAL
	{
		std::string out(getDecodedSize(enc.data(), enc.size()), '\0');
		decode(out.data(), enc.data(), enc.size(), table);
		return out;
	}

	void base64::decode(char* out, const char* data, size_t size, const unsigned char table[256]) noexcept
	{
		// Ignore pad bytes
		while (size != 0 && data[size - 1] == '=')
		{
			size--;
		}

		size_t i = 0;
		size_t j = 0;
		const size_t aligned_size = (size / 4) * 4;
		while (i != aligned_size)
		{
			uint32_t a = table[static_cast<uint8_t>(data[i++])];
			uint32_t b = table[static_cast<uint8_t>(data[i++])];
			uint32_t c = table[static_cast<uint8_t>(data[i++])];
			uint32_t d = table[static_cast<uint8_t>(data[i++])];
			uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);
			out[j++] = (triple >> 2 * 8) & 0xFF;
			out[j++] = (triple >> 1 * 8) & 0xFF;
			out[j++] = (triple >> 0 * 8) & 0xFF;
		}

		if (auto remainder = (size % 4))
		{
			auto extra = (remainder - 1) + (remainder == 1);

			uint32_t a = i != size ? table[static_cast<uint8_t>(data[i++])] : 0;
			uint32_t b = i != size ? table[static_cast<uint8_t>(data[i++])] : 0;
			uint32_t c = i != size ? table[static_cast<uint8_t>(data[i++])] : 0;
			uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6);
			if (extra--) out[j++] = (triple >> 2 * 8) & 0xFF;
			if (extra--) out[j++] = (triple >> 1 * 8) & 0xFF;
		}
	}
}
