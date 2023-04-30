#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

namespace soup
{
	class Reader : public ioBase<true>
	{
	public:
		using ioBase::ioBase;

	protected:
		virtual bool str_impl(std::string& v, size_t len) = 0;

	public:
		[[nodiscard]] virtual bool hasMore() = 0;

		bool skip(size_t len)
		{
			std::string v;
			return str(len, v);
		}

		// An unsigned 64-bit integer encoded in 1..9 bytes. The most significant bit of bytes 1 to 8 is used to indicate if another byte follows.
		bool u64_dyn(uint64_t& v)
		{
			v = 0;
			uint8_t bits = 0;
			while (true)
			{
				uint8_t b;
				if (!u8(b))
				{
					return false;
				}
				bool has_next = ((b >> 7) & (bits < 56));
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

		// A signed 64-bit integer encoded in 1..9 bytes.
		bool i64_dyn(int64_t& v)
		{
			bool neg;
			uint8_t b;
			if (!u8(b))
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
					if (!u8(b))
					{
						return false;
					}
					bool has_next = ((b >> 7) & (bits < 55));
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

		// An integer where every byte's most significant bit is used to indicate if another byte follows.
		template <typename Int>
		bool om(Int& v)
		{
			Int val{};
			while (hasMore())
			{
				uint8_t byte;
				u8(byte);
				val <<= 7;
				val |= (byte & 0x7F);
				if (!(byte & 0x80))
				{
					break;
				}
			}
			v = val;
			return true;
		}

		// Specialisation of the above for Bigint.
		bool om_bigint(Bigint& v);

		bool mysql_lenenc(uint64_t& v)
		{
			uint8_t first;
			if (u8(first))
			{
				if (first < 0xFB)
				{
					v = first;
					return true;
				}
				// 0xFB = NULL
				if (first == 0xFC)
				{
					uint16_t val;
					if (u16(val))
					{
						v = val;
						return true;
					}
				}
				if (first == 0xFD)
				{
					uint32_t val;
					if (u24(val))
					{
						v = val;
						return true;
					}
				}
				if (first == 0xFE)
				{
					uint64_t val;
					if (u64(val))
					{
						v = val;
						return true;
					}
				}
				// 0xFF = undocumented
			}
			return false;
		}

		// Null-terminated string.
		bool str_nt(std::string& v)
		{
			v.clear();
			while (true)
			{
				char c;
				if (!ioBase::c(c))
				{
					return false;
				}
				if (c == 0)
				{
					break;
				}
				v.push_back(c);
			}
			return true;
		}

		// Length-prefixed string, using u64_dyn for the length prefix.
		bool str_lp_u64_dyn(std::string& v)
		{
			uint64_t len;
			return u64_dyn(len) && str_impl(v, (size_t)len);
		}

		bool bigint_lp_u64_dyn(Bigint& v);

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(std::string& v)
		{
			uint64_t len;
			return mysql_lenenc(len) && str_impl(v, (size_t)len);
		}

		// Length-prefixed string, using u8 for the length prefix.
		bool str_lp_u8(std::string& v, const uint8_t max_len = 0xFF)
		{
			uint8_t len;
			return u8(len) && len <= max_len && str_impl(v, len);
		}

		// Length-prefixed string, using u16 for the length prefix.
		bool str_lp_u16(std::string& v, const uint16_t max_len = 0xFFFF)
		{
			uint16_t len;
			return ioBase::u16(len) && len <= max_len && str_impl(v, len);
		}

		// Length-prefixed string, using u24 for the length prefix.
		bool str_lp_u24(std::string& v, const uint32_t max_len = 0xFFFFFF)
		{
			uint32_t len;
			return ioBase::u24(len) && len <= max_len && str_impl(v, len);
		}

		// Length-prefixed string, using u32 for the length prefix.
		bool str_lp_u32(std::string& v, const uint32_t max_len = 0xFFFFFFFF)
		{
			uint32_t len;
			return ioBase::u32(len) && len <= max_len && str_impl(v, len);
		}

		// Length-prefixed string, using u64 for the length prefix.
		bool str_lp_u64(std::string& v)
		{
			uint64_t len;
			return ioBase::u64(len) && str_impl(v, (size_t)len);
		}

		// String with known length.
		bool str(size_t len, std::string& v)
		{
			return str_impl(v, len);
		}

		// std::vector<uint8_t> with u8 size prefix.
		bool vec_u8_u8(std::vector<uint8_t>& v)
		{
			uint8_t len;
			if (!u8(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len);
			for (; len; --len)
			{
				uint8_t entry;
				if (!u8(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// std::vector<uint16_t> with u16 size prefix.
		bool vec_u16_u16(std::vector<uint16_t>& v)
		{
			uint16_t len;
			if (!ioBase::u16(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 2);
			for (; len; --len)
			{
				uint16_t entry;
				if (!ioBase::u16(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// std::vector<uint16_t> with u16 byte length prefix.
		bool vec_u16_bl_u16(std::vector<uint16_t>& v)
		{
			uint16_t len;
			if (!ioBase::u16(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 2);
			for (; len >= sizeof(uint16_t); len -= sizeof(uint16_t))
			{
				uint16_t entry;
				if (!ioBase::u16(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_nt with u64_dyn length prefix.
		bool vec_str_nt_u64_dyn(std::vector<std::string>& v)
		{
			uint64_t len;
			if (!u64_dyn(len))
			{
				return false;
			}
			v.clear();
			v.reserve((size_t)len);
			for (; len != 0; --len)
			{
				std::string entry;
				if (!str_nt(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_lp_u24 with u24 byte length prefix.
		bool vec_str_lp_u24_bl_u24(std::vector<std::string>& v)
		{
			uint32_t len;
			if (!ioBase::u24(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 3);
			while (len >= 3)
			{
				std::string entry;
				if (!str_lp_u24(entry))
				{
					return false;
				}
				len -= ((uint32_t)entry.size() + 3);
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// Null-terminated vector of str_lp_u8.
		bool vec_nt_str_lp_u8(std::vector<std::string>& v)
		{
			v.clear();
			while (true)
			{
				std::string entry;
				if (!str_lp_u8(entry))
				{
					return false;
				}
				if (entry.empty())
				{
					break;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// Reader-specific
		virtual bool getLine(std::string& line) noexcept
		{
			line.clear();
			char c;
			while (ioBase::c(c))
			{
				SOUP_IF_UNLIKELY (c == '\n')
				{
					return true;
				}
				line.push_back(c);
			}
			return false;
		}
	};
}
