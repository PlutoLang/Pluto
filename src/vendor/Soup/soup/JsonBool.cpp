#include "JsonBool.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	void JsonBool::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		if (value)
		{
			str.append("true");
		}
		else
		{
			str.append("false");
		}
	}

	bool JsonBool::msgpackEncode(Writer& w) const
	{
		uint8_t b = 0xc2 + value;
		return w.u8(b);
	}
}
