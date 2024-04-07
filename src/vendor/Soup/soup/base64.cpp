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

	std::string base64::encode(const char* const data, const size_t size, const bool pad, const char* table) SOUP_EXCAL
	{
		size_t out_len = 4 * ((size + 2) / 3);
		std::string enc(out_len, '\0');
		size_t i = 0;
		char* p = enc.data();

		if (size > 2)
		{
			for (; i < size - 2; i += 3)
			{
				*p++ = table[(data[i] >> 2) & 0x3F];
				*p++ = table[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*p++ = table[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
				*p++ = table[data[i + 2] & 0x3F];
			}
		}
		if (i < size)
		{
			*p++ = table[(data[i] >> 2) & 0x3F];
			if (i == (size - 1))
			{
				*p++ = table[((data[i] & 0x3) << 4)];
				if (pad)
				{
					*p++ = '=';
				}
			}
			else
			{
				*p++ = table[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*p++ = table[((data[i + 1] & 0xF) << 2)];
			}
			if (pad)
			{
				*p++ = '=';
			}
		}
		if (!pad)
		{
			enc.erase(strlen(enc.c_str()));
		}

		return enc;
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

	std::string base64::decode(std::string enc) SOUP_EXCAL
	{
		return decode(std::move(enc), table_decode_base64);
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

	std::string base64::urlDecode(std::string enc) SOUP_EXCAL
	{
		return decode(std::move(enc), table_decode_base64url);
	}

	std::string base64::decode(std::string&& enc, const unsigned char* table) SOUP_EXCAL
	{
		std::string out{};
		if (!enc.empty())
		{
			size_t in_len;
			while (in_len = enc.size(), in_len % 4 != 0)
			{
				enc.push_back('=');
			}

			size_t out_len = in_len / 4 * 3;
			if (enc[in_len - 1] == '=') out_len--;
			if (enc[in_len - 2] == '=') out_len--;

			out.resize(out_len);

			for (size_t i = 0, j = 0; i < in_len; )
			{
				uint32_t a = enc[i] == '=' ? 0 & i++ : table[static_cast<uint8_t>(enc[i++])];
				uint32_t b = enc[i] == '=' ? 0 & i++ : table[static_cast<uint8_t>(enc[i++])];
				uint32_t c = enc[i] == '=' ? 0 & i++ : table[static_cast<uint8_t>(enc[i++])];
				uint32_t d = enc[i] == '=' ? 0 & i++ : table[static_cast<uint8_t>(enc[i++])];

				uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

				if (j < out_len) out[j++] = (triple >> 2 * 8) & 0xFF;
				if (j < out_len) out[j++] = (triple >> 1 * 8) & 0xFF;
				if (j < out_len) out[j++] = (triple >> 0 * 8) & 0xFF;
			}
		}
		return out;
	}
}
