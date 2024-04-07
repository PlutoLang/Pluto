#include "JsonBool.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonBool::JsonBool() noexcept
		: JsonNode(JSON_BOOL)
	{
	}

	JsonBool::JsonBool(bool value) noexcept
		: JsonNode(JSON_BOOL), value(value)
	{
	}

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

	bool JsonBool::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_BOOL;
		b |= (value << 3);
		return w.u8(b);
	}
}
