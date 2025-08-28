#include "Reader.hpp"

NAMESPACE_SOUP
{
	bool Reader::u64_dyn(uint64_t& v) noexcept
	{
		v = 0;
		uint8_t b;
		uint8_t bits = 0;
		for (uint8_t i = 0; i != 8; ++i)
		{
			SOUP_RETHROW_FALSE(u8(b));
			v += (uint64_t)(b & 0x7f) << bits;
			if (!(b >> 7))
			{
				return true;
			}
			bits += 7;
		}
		SOUP_RETHROW_FALSE(u8(b));
		v += (uint64_t)b << 56;
		return true;
	}

	bool Reader::i64_dyn(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		v = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		if (neg)
		{
			v = static_cast<int64_t>(~(v - 1) | (static_cast<uint64_t>(1) << 63));
		}
		return true;
	}

	bool Reader::u64_dyn_v2(uint64_t& v) noexcept
	{
		v = 0;
		uint8_t b;
		uint8_t bits = 0;
		for (uint8_t i = 0; i != 8; ++i)
		{
			SOUP_RETHROW_FALSE(u8(b));
			v += (uint64_t)(b & 0x7f) << bits;
			if (!(b >> 7))
			{
				return true;
			}
			bits += 7;
			v += (uint64_t)1 << bits; // v2
		}
		SOUP_RETHROW_FALSE(u8(b));
		v += (uint64_t)b << 56;
		return true;
	}

	bool Reader::i64_dyn_v2(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_RETHROW_FALSE(u64_dyn_v2(u));
		const bool neg = (u >> 6) & 1; // check bit 6
		u = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		if (neg)
		{
			v = ~u;
		}
		else
		{
			v = u;
		}
		return true;
	}
}
