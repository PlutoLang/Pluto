#include "JsonNull.hpp"

#include "Writer.hpp"

namespace soup
{
	JsonNull::JsonNull() noexcept
		: JsonNode(JSON_NULL)
	{
	}

	std::string JsonNull::encode() const
	{
		return "null";
	}

	bool JsonNull::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_NULL;
		return w.u8(b);
	}
}
