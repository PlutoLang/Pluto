#include "MacAddr.hpp"

#include "string.hpp"

NAMESPACE_SOUP
{
	MacAddr MacAddr::SOUP_FIRST = MacAddr(0x8C, 0x1F, 0x64, 0xB9, 0x85, 0x00);
	MacAddr MacAddr::SOUP_LAST = MacAddr(0x8C, 0x1F, 0x64, 0xB9, 0x85, 0xFF);

	std::string MacAddr::toString(char sep) const
	{
		std::string str(17, '\0');
		toString(str.data(), sep);
		return str;
	}

	void MacAddr::toString(char out[17], char sep /*= '-'*/) const
	{
		out[0] = string::charset_hex[data[0] >> 4];
		out[1] = string::charset_hex[data[0] & 0xF];
		out[2] = sep;
		out[3] = string::charset_hex[data[1] >> 4];
		out[4] = string::charset_hex[data[1] & 0xF];
		out[5] = sep;
		out[6] = string::charset_hex[data[2] >> 4];
		out[7] = string::charset_hex[data[2] & 0xF];
		out[8] = sep;
		out[9] = string::charset_hex[data[3] >> 4];
		out[10] = string::charset_hex[data[3] & 0xF];
		out[11] = sep;
		out[12] = string::charset_hex[data[4] >> 4];
		out[13] = string::charset_hex[data[4] & 0xF];
		out[14] = sep;
		out[15] = string::charset_hex[data[5] >> 4];
		out[16] = string::charset_hex[data[5] & 0xF];
	}
}
