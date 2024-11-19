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
			SOUP_RETHROW_FALSE(u8(b));
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
		SOUP_RETHROW_FALSE(u64_dyn(u));
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

	bool Reader::u64_dyn_v2(uint64_t& v) noexcept
	{
		uint8_t b;
		SOUP_RETHROW_FALSE(u8(b));
		v = (b & 0x7f);

		uint8_t bits = 7;
		bool has_next = (b >> 7);
		while (has_next)
		{
			SOUP_RETHROW_FALSE(u8(b));
			has_next = false;
			SOUP_IF_LIKELY (bits < 56)
			{
				has_next = (b >> 7);
				b &= 0x7f;
			}
			v |= (((uint64_t)b + 1) << bits);
			bits += 7;
		}

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
			v = (u * -1) - 1;
		}
		else
		{
			v = u;
		}
		return true;
	}
}
