#include "JsonFloat.hpp"

#include "string.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonFloat::JsonFloat(double value) noexcept
		: JsonNode(JSON_FLOAT), value(value)
	{
	}

	void JsonFloat::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.append(string::fdecimal(value));
	}

	bool JsonFloat::binaryEncode(Writer& w) const
	{
		uint8_t b = JSON_FLOAT;
		uint64_t val;
		*reinterpret_cast<double*>(&val) = value;
		return w.u8(b)
			&& w.u64(val)
			;
	}
}
