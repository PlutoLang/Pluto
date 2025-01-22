#include "Writer.hpp"

NAMESPACE_SOUP
{
	bool Writer::u64_dyn(const uint64_t& v) noexcept
	{
		bool ret = true;
		uint64_t in = v;
		for (uint8_t i = 0; i != 8; ++i)
		{
			uint8_t cur = (in & 0x7f);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				ret &= u8(cur);
			}
			else
			{
				ret &= u8(cur);
				return ret;
			}
		}
		if (in != 0)
		{
			auto byte = (uint8_t)in;
			ret &= u8(byte);
		}
		return ret;
	}

	bool Writer::i64_dyn(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		if (neg)
		{
			u = (v * -1) & ~((uint64_t)1 << 63);
		}
		else
		{
			u = v;
		}
		return u64_dyn(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
	}

	bool Writer::u64_dyn_v2(const uint64_t& v) noexcept
	{
		bool ret = true;
		uint64_t in = v;
		uint8_t cur;
		for (uint8_t i = 0; i != 8; ++i)
		{
			cur = (in & 0x7f);
			in >>= 7;
			if (in != 0)
			{
				cur |= 0x80;
				ret &= u8(cur);
				in -= 1; // v2
			}
			else
			{
				ret &= u8(cur);
				return ret;
			}
		}
		if (cur >> 7) // Last byte indicated another one would follow?
		{
			cur = (uint8_t)in;
			ret &= u8(cur);
		}
		return ret;
	}

	bool Writer::i64_dyn_v2(const int64_t& v) noexcept
	{
		uint64_t u;
		bool neg = (v < 0);
		if (neg)
		{
			u = (v * -1) - 1;
		}
		else
		{
			u = v;
		}
		return u64_dyn_v2(((uint64_t)neg << 6) | ((u & ~0x3f) << 1) | (u & 0x3f));
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
			return u8(prefix) && u16le(val);
		}
		else if (v <= 0xFFFFFF)
		{
			uint8_t prefix = 0xFD;
			auto val = (uint32_t)v;
			return u8(prefix) && u24le(val);
		}
		uint8_t prefix = 0xFE;
		auto val = v;
		return u8(prefix) && u64le(val);
	}
}
