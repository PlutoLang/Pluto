#include "base32.hpp"

#include <algorithm>
#include <vector>

#include "bitutil.hpp"

NAMESPACE_SOUP
{
	static const char b32_alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

	std::string base32::encode(const std::string& in, bool pad)
	{
		return encode(in, pad, b32_alpha);
	}

	std::string base32::encode(const std::string& in, bool pad, const char* alpha)
	{
		auto chunks = bitutil::msb_first<std::string, 8, 5>(in);
		for (auto& chunk : chunks)
		{
			chunk = alpha[(uint8_t)chunk];
		}
		if (pad)
		{
			if (auto padlen = getEncodedLength(in.length()) - chunks.length())
			{
				chunks.append(padlen, '=');
			}
		}
		return chunks;
	}

	static int decode_char(unsigned char c)
	{
		if (c >= 'A' && c <= 'Z')
		{
			return c - 'A';
		}
		if (c >= '2' && c <= '7')
		{
			return c - '2' + 26;
		}
		return -1;
	}

	static int get_octet(int block)
	{
		return (block * 5) / 8;
	}

	static int get_offset(int block)
	{
		return (8 - 5 - (5 * block) % 8);
	}

	static unsigned char shift_left(unsigned char byte, char offset)
	{
		if (offset < 0)
		{
			return byte >> -offset;
		}
		return byte << offset;
	}

	static bool decode_sequence(const uint8_t* coded, size_t octet_base, std::string& out)
	{
		for (int block = 0; block != 8; ++block)
		{
			int offset = get_offset(block);
			size_t octet = octet_base + get_octet(block);

			int c = decode_char(coded[block]);
			if (c == -1)
			{
				return false;
			}

			if (out.size() == octet)
			{
				out.push_back(shift_left(c, offset));
			}
			else
			{
				/*if (octet > out.size())
				{
					out.append(octet - out.size(), 0);
				}*/
				out.at(octet) |= shift_left(c, offset);
			}

			if (offset < 0) // does this block overflow to next octet?
			{
				auto overflow = shift_left(c, 8 + offset);
				if (overflow != 0)
				{
					out.push_back(overflow);
				}
			}
		}
		return true;
	}

	std::string base32::decode(const std::string& in)
	{
		std::string out{};
		if (!in.empty())
		{
			auto arr = (const uint8_t*)&in.at(0);
			out.reserve(getDecodedLength(in.size()));
			for (size_t i = 0, j = 0; ; i += 8, j += 5)
			{
				if (!decode_sequence(&arr[i], j, out))
				{
					break;
				}
			}
		}
		return out;
	}
}
