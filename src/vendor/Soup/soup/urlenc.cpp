#include "urlenc.hpp"

#include "string.hpp"

NAMESPACE_SOUP
{
#define UNRESERVED case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '-': case '_': case '.': case '~':

	static void encode_percent(std::string& res, uint8_t b) SOUP_EXCAL
	{
		res.push_back('%');
		res.push_back(string::charset_hex[b >> 4]);
		res.push_back(string::charset_hex[b & 0xF]);
	}

	std::string urlenc::encode(const std::string& data) SOUP_EXCAL
	{
		std::string res{};
		for (const auto& c : data)
		{
			switch (c)
			{
			UNRESERVED
				res.push_back(c);
				break;

			default:
				encode_percent(res, c);
			}
		}
		return res;
	}

	std::string urlenc::encodePath(const std::string& data) SOUP_EXCAL
	{
		std::string res{};
		for (const auto& c : data)
		{
			switch (c)
			{
			UNRESERVED
			case '/': case '@':
				res.push_back(c);
				break;

			default:
				encode_percent(res, c);
			}
		}
		return res;
	}

	std::string urlenc::encodePathWithQuery(const std::string& data) SOUP_EXCAL
	{
		std::string res{};
		for (const auto& c : data)
		{
			switch (c)
			{
			UNRESERVED
			case '/': case '@':
			case '?': case '=': case '&':
				res.push_back(c);
				break;

			default:
				encode_percent(res, c);
			}
		}
		return res;
	}

	std::string urlenc::decode(const std::string& data) SOUP_EXCAL
	{
		std::string res;
		for (auto i = data.begin(); i != data.end(); )
		{
			if (*i == '%' && (i + 1) != data.end() && (i + 2) != data.end())
			{
				std::string hex;
				hex.push_back(*(++i));
				hex.push_back(*(++i));
				++i;
				if (unsigned char c; string::hexToIntOpt<unsigned char>(hex).consume(c))
				{
					res.push_back(static_cast<char>(c));
				}
				else
				{
					i -= 2;
					res.push_back('%');
				}
			}
			else
			{
				res.push_back(*i);
				++i;
			}
		}
		return res;
	}
}
