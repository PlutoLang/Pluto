#include "JsonString.hpp"

#include "string.hpp"
#include "unicode.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonString::JsonString() noexcept
		: JsonNode(JSON_STRING)
	{
	}

	JsonString::JsonString(const std::string& value) noexcept
		: JsonNode(JSON_STRING), value(value)
	{
	}

	JsonString::JsonString(std::string&& value) noexcept
		: JsonNode(JSON_STRING), value(std::move(value))
	{
	}

	JsonString::JsonString(const char*& c) SOUP_EXCAL
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
						if (char32_t w1; string::hexToIntOpt<char32_t>(std::string(c, 4)).consume(w1))
						{
							c += 4;
							if ((w1 >> 10) == 0x36) // Surrogate pair?
							{
								if (c[0] == '\\' && c[1] == 'u' && c[2] && c[3] && c[4] && c[5])
								{
									c += 2;
									if (char32_t w2; string::hexToIntOpt<char32_t>(std::string(c, 4)).consume(w2))
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

	bool JsonString::operator==(const JsonNode& b) const noexcept
	{
		return JSON_STRING == b.type
			&& value == b.reinterpretAsStr().value
			;
	}

	void JsonString::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
#if !SOUP_LINUX // std::string::reserve is seemingly misimplemented (relative instead of absolute)
		str.reserve(str.size() + value.size() + 2);
#endif
		str.push_back('"');
		for (const auto& c : value)
		{
			switch (c)
			{
			default:
				str.push_back(c);
				break;

			case 0x00: str.append("\\u0000"); break;
			case 0x01: str.append("\\u0001"); break;
			case 0x02: str.append("\\u0002"); break;
			case 0x03: str.append("\\u0003"); break;
			case 0x04: str.append("\\u0004"); break;
			case 0x05: str.append("\\u0005"); break;
			case 0x06: str.append("\\u0006"); break;
			case 0x07: str.append("\\u0007"); break;
			case 0x08: str.append("\\u0008"); break;
			case 0x09: str.append("\\t"); static_assert('\t' == 0x09); break;
			case 0x0A: str.append("\\n"); static_assert('\n' == 0x0A); break;
			case 0x0B: str.append("\\u000B"); break;
			case 0x0C: str.append("\\u000C"); break;
			case 0x0D: str.append("\\r"); static_assert('\r' == 0x0D); break;
			case 0x0E: str.append("\\u000E"); break;
			case 0x0F: str.append("\\u000F"); break;
			case 0x10: str.append("\\u0010"); break;
			case 0x11: str.append("\\u0011"); break;
			case 0x12: str.append("\\u0012"); break;
			case 0x13: str.append("\\u0013"); break;
			case 0x14: str.append("\\u0014"); break;
			case 0x15: str.append("\\u0015"); break;
			case 0x16: str.append("\\u0016"); break;
			case 0x17: str.append("\\u0017"); break;
			case 0x18: str.append("\\u0018"); break;
			case 0x19: str.append("\\u0019"); break;
			case 0x1A: str.append("\\u001A"); break;
			case 0x1B: str.append("\\u001B"); break;
			case 0x1C: str.append("\\u001C"); break;
			case 0x1D: str.append("\\u001D"); break;
			case 0x1E: str.append("\\u001E"); break;
			case 0x1F: str.append("\\u001F"); break;

			case '\\':
				str.append("\\\\");
				break;

			case '\"':
				str.append("\\\"");
				break;
			}
		}
		str.push_back('"');
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
