#include "base58.hpp"

#include <vector>

#include "alpha2decodetbl.hpp"
#include "Exception.hpp"

namespace soup
{
	std::string base58::decode(const std::string& in)
	{
		auto mapBase58 = alpha2decodetbl("123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz");

		const char* psz = in.c_str();

		// Skip and count leading '1's.
		int zeroes = 0;
		int length = 0;
		while (*psz == '1') {
			zeroes++;
			psz++;
		}
		// Allocate enough space in big-endian base256 representation.
		auto size = in.length() * 733 / 1000 + 1; // log(58) / log(256), rounded up.
		std::vector<uint8_t> b256(size);
		// Process the characters.
		while (*psz)
		{
			// Decode base58 character
			int carry = mapBase58[(uint8_t)*psz];
			if (carry == 0xFF)
			{
				throw Exception("Invalid base58 character");
			}
			int i = 0;
			for (std::vector<uint8_t>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
				carry += 58 * (*it);
				*it = carry % 256;
				carry /= 256;
			}
			length = i;
			psz++;
		}
		// Skip leading zeroes in b256.
		std::vector<uint8_t>::iterator it = b256.begin() + (size - length);
		// Copy result into output vector.
		std::string out;
		out.reserve(zeroes + (b256.end() - it));
		out.assign(zeroes, 0x00);
		while (it != b256.end())
		{
			out.push_back(*(it++));
		}
		return out;
	}
}
