#include "JsonFloat.hpp"

#include "string.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	void JsonFloat::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.append(string::fdecimal(value));
	}

	bool JsonFloat::msgpackEncode(Writer& w) const
	{
		if (auto fval = (float)value; value == (double)fval) // Can be represented as f32 without precision loss?
		{
			uint8_t b = 0xca;
			return w.u8(b)
				&& w.f32(fval)
				;
		}

		uint8_t b = 0xcb;
		return w.u8(b)
			&& w.f64(const_cast<JsonFloat*>(this)->value)
			;
	}
}
