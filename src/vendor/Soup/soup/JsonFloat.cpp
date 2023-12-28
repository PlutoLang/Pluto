#include "JsonFloat.hpp"

#include "string.hpp"
#include "Writer.hpp"

namespace soup
{
	JsonFloat::JsonFloat(double value) noexcept
		: JsonNode(JSON_FLOAT), value(value)
	{
	}

	std::string JsonFloat::encode() const
	{
		return string::fdecimal(value);
	}

	bool JsonFloat::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_FLOAT;
		uint64_t val;
		memcpy(&val, &value, sizeof(double)); static_assert(sizeof(uint64_t) == sizeof(value));
		return w.u8(b)
			&& w.u64(val)
			;
	}
}
