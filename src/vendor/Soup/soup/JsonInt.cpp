#include "JsonInt.hpp"

#include "Writer.hpp"

NAMESPACE_SOUP
{
	bool JsonInt::operator==(const JsonNode& b) const noexcept
	{
		return JSON_INT == b.getType()
			&& value == b.reinterpretAsInt().value
			;
	}

	void JsonInt::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.append(std::to_string(value));
	}

	bool JsonInt::msgpackEncode(Writer& w) const
	{
		if (value >= 0)
		{
			if (value < 0x80)
			{
				uint8_t b = (uint8_t)value;
				return w.u8(b);
			}
			if (value <= 0xff)
			{
				uint8_t b = 0xcc;
				auto val = (uint8_t)value;
				return w.u8(b)
					&& w.u8(val)
					;
			}
			if (value <= 0xffff)
			{
				uint8_t b = 0xcd;
				auto val = (uint16_t)value;
				return w.u8(b)
					&& w.u16_be(val)
					;
			}
			if (value <= 0xffff'ffff)
			{
				uint8_t b = 0xce;
				auto val = (uint32_t)value;
				return w.u8(b)
					&& w.u32_be(val)
					;
			}
			uint8_t b = 0xcf;
			uint64_t val = value;
			return w.u8(b)
				&& w.u64_be(val)
				;
		}
		else
		{
			if (value >= -32)
			{
				int8_t b = (int8_t)value;
				return w.i8(b);
			}
			if (value >= -128)
			{
				uint8_t b = 0xd0;
				auto val = (int8_t)value;
				return w.u8(b)
					&& w.i8(val)
					;
			}
			if (value >= -32768)
			{
				uint8_t b = 0xd1;
				auto val = (int16_t)value;
				return w.u8(b)
					&& w.i16_be(val)
					;
			}
			if (value >= -2147483648)
			{
				uint8_t b = 0xd2;
				auto val = (int32_t)value;
				return w.u8(b)
					&& w.i32_be(val)
					;
			}
			uint8_t b = 0xd3;
			return w.u8(b)
				&& w.i64_be(const_cast<JsonInt*>(this)->value)
				;
		}
	}
}
