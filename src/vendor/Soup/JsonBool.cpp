#include "JsonBool.hpp"

#include "Writer.hpp"

namespace soup
{
	JsonBool::JsonBool() noexcept
		: JsonNode(JSON_BOOL)
	{
	}

	JsonBool::JsonBool(bool value) noexcept
		: JsonNode(JSON_BOOL), value(value)
	{
	}

	std::string JsonBool::encode() const
	{
		return value ? "true" : "false";
	}

	bool JsonBool::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_BOOL;
		b |= (value << 3);
		return w.u8(b);
	}
}
