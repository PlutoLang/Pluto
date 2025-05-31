#include "JsonNull.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonNull::JsonNull() noexcept
		: JsonNode(JSON_NULL)
	{
	}

	void JsonNull::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.append("null");
	}

	bool JsonNull::msgpackEncode(Writer& w) const
	{
		uint8_t b = 0xc0;
		return w.u8(b);
	}
}
