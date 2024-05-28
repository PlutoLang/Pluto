#include "Writer.hpp"

NAMESPACE_SOUP
{
	bool Writer::u64_dyn(const uint64_t& v) noexcept
	{
		uint64_t in = v;
		for (uint8_t i = 0; i != 8; ++i)
		{
			uint8_t cur = (in & 0x7F);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				u8(cur);
			}
			else
			{
				return u8(cur);
			}
		}
		if (in != 0)
		{
			auto byte = (uint8_t)in;
			return u8(byte);
		}
		return true;
	}

	bool Writer::i64_dyn(const int64_t& v) noexcept
	{
		// Split value
		uint64_t in;
		bool neg = (v < 0);
		if (neg)
		{
			in = (v * -1);
			in &= ~((uint64_t)1 << 63);
		}
		else
		{
			in = v;
		}

		// First byte
		{
			uint8_t cur = (in & 0b111111);
			cur |= (neg << 6);
			in >>= 6;
			if (in != 0)
			{
				cur |= 0x80;
				u8(cur);
			}
			else
			{
				return u8(cur);
			}
		}

		// Next 1..7 bytes
		for (uint8_t i = 0; i != 7; ++i)
		{
			uint8_t cur = (in & 0x7F);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				u8(cur);
			}
			else
			{
				return u8(cur);
			}
		}

		// Optional last byte
		if (in != 0)
		{
			auto byte = (uint8_t)in;
			return u8(byte);
		}
		return true;
	}

	bool Writer::mysql_lenenc(const uint64_t& v) noexcept
	{
		if (v < 0xFB)
		{
			auto val = (uint8_t)v;
			return u8(val);
		}
		else if (v <= 0xFFFF)
		{
			uint8_t prefix = 0xFC;
			auto val = (uint16_t)v;
			return u8(prefix) && u16_le(val);
		}
		else if (v <= 0xFFFFFF)
		{
			uint8_t prefix = 0xFD;
			auto val = (uint32_t)v;
			return u8(prefix) && u24_le(val);
		}
		uint8_t prefix = 0xFE;
		auto val = v;
		return u8(prefix) && u64_le(val);
	}
}
