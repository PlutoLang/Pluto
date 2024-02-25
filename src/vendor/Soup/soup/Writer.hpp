#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

namespace soup
{
	class Writer : public ioBase<false>
	{
	public:
		using ioBase::ioBase;

		bool skip(size_t len)
		{
			uint8_t v = 0;
			while (len-- != 0)
			{
				u8(v);
			}
			return true;
		}

		// An unsigned 64-bit integer encoded in 1..9 bytes. The most significant bit of bytes 1 to 8 is used to indicate if another byte follows.
		bool u64_dyn(const uint64_t& v)
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
					u8(cur);
					return true;
				}
			}
			if (in != 0)
			{
				auto byte = (uint8_t)in;
				u8(byte);
			}
			return true;
		}

		// A signed 64-bit integer encoded in 1..9 bytes.
		bool i64_dyn(const int64_t& v)
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
					u8(cur);
					return true;
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
					u8(cur);
					return true;
				}
			}

			// Optional last byte
			if (in != 0)
			{
				auto byte = (uint8_t)in;
				u8(byte);
			}
			return true;
		}

		// An integer where every byte's most significant bit is used to indicate if another byte follows.
		template <typename Int>
		bool om(const Int& v)
		{
			auto chunks = 0;
			{
				Int val = v;
				while (true)
				{
					val >>= 7;
					if (!val)
					{
						break;
					}
					++chunks;
				}
			}
			do
			{
				uint8_t byte = ((v >> (chunks * 7)) & 0x7F);
				if (chunks != 0)
				{
					byte |= 0x80;
				}
				u8(byte);
			} while (chunks--);
			return true;
		}

		// Specialisation of the above for Bigint.
		// There are better ways to encode Bigints, tho, such as bigint_lp_u64_dyn.
		bool om_bigint(const Bigint& v);

		bool mysql_lenenc(const uint64_t& v)
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

		// Null-terminated string.
		bool str_nt(const std::string& v)
		{
			raw(const_cast<char*>(v.data()), v.size());
			uint8_t term = 0;
			u8(term);
			return true;
		}

		// Length-prefixed string.
		template <typename T>
		bool str_lp(const std::string& v, const T max_len = -1)
		{
			size_t len = v.size();
			if (len <= max_len)
			{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4267) // possible loss of data
#endif
				auto tl = static_cast<T>(len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
				ser<T>(tl);
				raw(const_cast<char*>(v.data()), v.size());
				return true;
			}
			return false;
		}

		// Length-prefixed string, using u64_dyn for the length prefix.
		bool str_lp_u64_dyn(const std::string& v)
		{
			u64_dyn(v.size());
			raw(const_cast<char*>(v.data()), v.size());
			return true;
		}

		bool bigint_lp_u64_dyn(Bigint& v);

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(const std::string& v)
		{
			mysql_lenenc(v.size());
			raw(const_cast<char*>(v.data()), v.size());
			return true;
		}

		// Length-prefixed string, using u8 for the length prefix.
		[[deprecated]] bool str_lp_u8(const std::string& v, const uint8_t max_len = 0xFF)
		{
			return str_lp<u8_t>(v, max_len);
		}

		// Length-prefixed string, using u16 for the length prefix.
		[[deprecated]] bool str_lp_u16(const std::string& v, const uint16_t max_len = 0xFFFF)
		{
			return str_lp<u16_t>(v, max_len);
		}

		// Length-prefixed string, using u24 for the length prefix.
		[[deprecated]] bool str_lp_u24(const std::string& v, const uint32_t max_len = 0xFFFFFF)
		{
			return str_lp<u24_t>(v, max_len);
		}

		// Length-prefixed string, using u32 for the length prefix.
		[[deprecated]] bool str_lp_u32(const std::string& v, const uint32_t max_len = 0xFFFFFFFF)
		{
			return str_lp<u32_t>(v, max_len);
		}

		// Length-prefixed string, using u64 for the length prefix.
		[[deprecated]] bool str_lp_u64(const std::string& v)
		{
			return str_lp<u64_t>(v);
		}

		// String with known length.
		bool str(size_t len, std::string v)
		{
			SOUP_IF_UNLIKELY (v.size() != len)
			{
				v.append(len - v.size(), '\0');
			}
			raw(v.data(), v.size());
			return true;
		}

		// String with known length.
		bool str(size_t len, const char* v)
		{
			raw(const_cast<char*>(v), len);
			return true;
		}

		// std::vector<uint8_t> with u8 size prefix.
		bool vec_u8_u8(std::vector<uint8_t>& v)
		{
			if (v.size() > 0xFF)
			{
				return false;
			}
			auto len = (uint8_t)v.size();
			if (!u8(len))
			{
				return false;
			}
			for (auto& entry : v)
			{
				if (!u8(entry))
				{
					return false;
				}
			}
			return true;
		}

		// std::vector<uint16_t> with u16 size prefix.
		bool vec_u16_u16(std::vector<uint16_t>& v)
		{
			if (v.size() > 0xFFFF)
			{
				return false;
			}
			auto len = (uint16_t)v.size();
			if (!ioBase::u16(len))
			{
				return false;
			}
			for (auto& entry : v)
			{
				if (!ioBase::u16(entry))
				{
					return false;
				}
			}
			return true;
		}

		// std::vector<uint16_t> with u16 byte length prefix.
		bool vec_u16_bl_u16(std::vector<uint16_t>& v)
		{
			size_t bl = (v.size() * sizeof(uint16_t));
			if (bl > 0xFFFF)
			{
				return false;
			}
			auto bl_u16 = (uint16_t)bl;
			if (!ioBase::u16(bl_u16))
			{
				return false;
			}
			for (auto& entry : v)
			{
				if (!ioBase::u16(entry))
				{
					return false;
				}
			}
			return true;
		}

		// vector of str_nt with u64_dyn length prefix.
		bool vec_str_nt_u64_dyn(std::vector<std::string>& v)
		{
			uint64_t len = v.size();
			if (!u64_dyn(len))
			{
				return false;
			}
			for (auto& entry : v)
			{
				if (!str_nt(entry))
				{
					return false;
				}
			}
			return true;
		}

		// vector of str_lp<u24_t> with u24 byte length prefix.
		bool vec_str_lp_u24_bl_u24(std::vector<std::string>& v)
		{
			size_t bl = (v.size() * 3);
			for (const auto& entry : v)
			{
				bl += entry.size();
			}
			if (bl > 0xFFFFFF)
			{
				return false;
			}
			auto bl_u32 = (uint32_t)bl;
			if (!ioBase::u24(bl_u32))
			{
				return false;
			}
			for (auto& entry : v)
			{
				if (!str_lp<u24_t>(entry))
				{
					return false;
				}
			}
			return true;
		}
	};
}
