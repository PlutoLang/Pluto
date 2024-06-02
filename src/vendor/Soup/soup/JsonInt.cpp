#include "JsonInt.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonInt::JsonInt(int64_t value) noexcept
		: JsonNode(JSON_INT), value(value)
	{
	}

	bool JsonInt::operator==(const JsonNode& b) const noexcept
	{
		return JSON_INT == b.type
			&& value == b.reinterpretAsInt().value
			;
	}

	void JsonInt::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.append(std::to_string(value));
	}

	bool JsonInt::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_INT;
		if (value >= 0 && value < 0b11111)
		{
			b |= (value << 3);
			return w.u8(b);
		}
		b |= (0b11111 << 3);
		return w.u8(b)
			&& w.i64_dyn(value)
			;
	}
}
