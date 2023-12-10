#include "JsonInt.hpp"

#include "Writer.hpp"

namespace soup
{
	JsonInt::JsonInt(int64_t value) noexcept
		: JsonNode(JSON_INT), value(value)
	{
	}

	std::string JsonInt::encode() const
	{
		return std::to_string(value);
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
