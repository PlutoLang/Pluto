#include "Reader.hpp"

#include "Bigint.hpp"

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
			bool has_next = (b >> 7); has_next &= (bits < 56);
			if (has_next)
			{
				b &= 0x7F;
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
		bool neg;
		uint8_t b;
		SOUP_IF_UNLIKELY (!u8(b))
		{
			return false;
		}
		uint64_t out = (b & 0b111111);
		neg = ((b >> 6) & 1);
		if ((b >> 7))
		{
			uint8_t bits = 6;
			while (true)
			{
				SOUP_IF_UNLIKELY (!u8(b))
				{
					return false;
				}
				bool has_next = (b >> 7); has_next &= (bits < 56);
				if (has_next)
				{
					b &= 0x7F;
				}
				out |= ((uint64_t)b << bits);
				if (!has_next)
				{
					break;
				}
				bits += 7;
			}
		}
		if (neg)
		{
			if (out == 0)
			{
				v = ((uint64_t)1 << 63);
			}
			else
			{
				v = (out * -1);
			}
		}
		else
		{
			v = out;
		}
		return true;
	}

	bool Reader::om_bigint(Bigint& v) SOUP_EXCAL
	{
		v.reset();
		uint8_t byte;
		while (u8(byte))
		{
			v <<= 7;
			v |= (Bigint::chunk_t)(byte & 0x7F);
			if (!(byte & 0x80))
			{
				return true;
			}
		}
		return false;
	}

	bool Reader::bigint_lp_u64_dyn(Bigint& v) SOUP_EXCAL
	{
		std::string str;
		SOUP_IF_LIKELY (str_lp_u64_dyn(str))
		{
			v = Bigint::fromBinary(str);
			return true;
		}
		return false;
	}
}
