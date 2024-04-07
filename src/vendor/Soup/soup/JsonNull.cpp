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

	bool JsonNull::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_NULL;
		return w.u8(b);
	}
}
