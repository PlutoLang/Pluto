#include "Reader.hpp"

NAMESPACE_SOUP
{
	bool Reader::u64_dyn(uint64_t& v) noexcept
	{
		v = 0;
		uint8_t bits = 0;
		while (true)
		{
			uint8_t b;
			SOUP_IF_UNLIKELY (!u8(b))
			{
				return false;
			}
			bool has_next = false;
			SOUP_IF_LIKELY (bits < 56)
			{
				has_next = (b >> 7);
				b &= 0x7f;
			}
			v |= ((uint64_t)b << bits);
			if (!has_next)
			{
				break;
			}
			bits += 7;
		}
		return true;
	}

	bool Reader::i64_dyn(int64_t& v) noexcept
	{
		uint64_t u;
		SOUP_IF_UNLIKELY (!u64_dyn(u))
		{
			return false;
		}
		const bool neg = (u >> 6) & 1; // check bit 6
		u = ((u >> 1) & ~0x3f) | (u & 0x3f); // remove bit 6
		if (neg)
		{
			if (u == 0)
			{
				v = ((uint64_t)1 << 63);
			}
			else
			{
				v = u * -1;
			}
		}
		else
		{
			v = u;
		}
		return true;
	}
}
