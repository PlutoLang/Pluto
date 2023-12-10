#include "JsonString.hpp"

#include "string.hpp"
#include "unicode.hpp"
#include "Writer.hpp"

namespace soup
{
	JsonString::JsonString() noexcept
		: JsonNode(JSON_STRING)
	{
	}

	JsonString::JsonString(std::string&& value) noexcept
		: JsonNode(JSON_STRING), value(std::move(value))
	{
	}

	JsonString::JsonString(const char*& c)
		: JsonString()
	{
		for (bool escaped = false; *c != 0; ++c)
		{
			if (escaped)
			{
				escaped = false;
				switch (*c)
				{
				default:
					value.push_back(*c);
					break;

				case 'n':
					value.push_back('\n');
					break;

				case 'r':
					value.push_back('\r');
					break;

				case 't':
					value.push_back('\t');
					break;

				case 'u':
					++c;
					if (c[0] && c[1] && c[2] && c[3])
					{
						if (char32_t w1; string::hexToInt<char32_t>(std::string(c, 4)).consume(w1))
						{
							c += 4;
							if ((w1 >> 10) == 0x36) // Surrogate pair?
							{
								if (c[0] == '\\' && c[1] == 'u' && c[2] && c[3] && c[4] && c[5])
								{
									c += 2;
									if (char32_t w2; string::hexToInt<char32_t>(std::string(c, 4)).consume(w2))
									{
										c += 4;
										value.append(unicode::utf32_to_utf8(unicode::utf16_to_utf32(w1, w2)));
									}
									else
									{
										c -= 2;
										c -= 4;
										value.push_back('u');
									}
								}
								else
								{
									c -= 4;
									value.push_back('u');
								}
							}
							else
							{
								value.append(unicode::utf32_to_utf8(w1));
							}
						}
						else
						{
							value.push_back('u');
						}
					}
					else
					{
						value.push_back('u');
					}
					--c;
					break;
				}
				continue;
			}
			if (*c == '"')
			{
				++c;
				break;
			}
			if (*c == '\\')
			{
				escaped = true;
				continue;
			}
			value.push_back(*c);
		}
	}

	std::string JsonString::encode() const
	{
		std::string str = *this;
		string::replaceAll(str, "\\", "\\\\");
		string::replaceAll(str, "\"", "\\\"");
		string::replaceAll(str, "\r", "\\r");
		string::replaceAll(str, "\n", "\\n");
		str.insert(0, 1, '"');
		str.push_back('"');
		return str;
	}

	bool JsonString::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_STRING;
		if (value.size() < 0b11111)
		{
			b |= ((uint8_t)value.size() << 3);
			return w.u8(b)
				&& w.str(value.size(), value)
				;
		}
		b |= (0b11111 << 3);
		return w.u8(b)
			&& w.str_lp_u64_dyn(*this)
			;
	}
}
