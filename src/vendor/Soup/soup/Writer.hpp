#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

namespace soup
{
	class Writer : public ioBase<false>
	{
	public:
		using ioBase::ioBase;

		bool skip(size_t len) noexcept
		{
			uint8_t v = 0;
			while (len--)
			{
				u8(v);
			}
			return true;
		}

		// An unsigned 64-bit integer encoded in 1..9 bytes. The most significant bit of bytes 1 to 8 is used to indicate if another byte follows.
		bool u64_dyn(const uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes.
		bool i64_dyn(const int64_t& v) noexcept;

		// An integer where every byte's most significant bit is used to indicate if another byte follows, most significant byte first.
		template <typename Int>
		bool om(const Int& v) noexcept
		{
			bool ret = true;
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
				ret &= u8(byte);
			} while (chunks--);
			return ret;
		}

		// Specialisation of the above for Bigint.
		// There are better ways to encode Bigints, tho, such as bigint_lp_u64_dyn.
		bool om_bigint(const Bigint& v) SOUP_EXCAL;

		// An integer where every byte's most significant bit is used to indicate if another byte follows, least significant byte first. This is compatible with unsigned LEB128.
		template <typename Int>
		bool oml(const Int& v) noexcept
		{
			bool ret = true;
			Int in = v;
			while (true)
			{
				uint8_t byte = (in & 0x7F);
				in >>= 7;
				if (in == 0)
				{
					ret &= u8(byte);
					break;
				}
				byte |= 0x80;
				ret &= u8(byte);
			}
			return ret;
		}

		// Signed LEB128.
		template <typename Int>
		bool soml(const Int& v) noexcept
		{
			bool ret = true;
			Int in = v;
			while (true)
			{
				uint8_t byte = (in & 0x7F);
				in >>= 7;
				if ((byte & 0x40) ? (in == -1) : (in == 0))
				{
					ret &= u8(byte);
					break;
				}
				byte |= 0x80;
				ret &= u8(byte);
			}
			return ret;
		}

		bool mysql_lenenc(const uint64_t& v) noexcept;

		// Null-terminated string.
		bool str_nt(const std::string& v) noexcept
		{
			bool ret = raw(const_cast<char*>(v.data()), v.size());
			uint8_t term = 0;
			ret &= u8(term);
			return ret;
		}

		// Length-prefixed string.
		template <typename T>
		bool str_lp(const std::string& v, const T max_len = -1) noexcept
		{
			size_t len = v.size();
			SOUP_IF_LIKELY (len <= max_len)
			{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4267) // possible loss of data
#endif
				auto tl = static_cast<T>(len);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
				bool ret = ser<T>(tl);
				ret &= raw(const_cast<char*>(v.data()), v.size());
				return ret;
			}
			return false;
		}

		// Length-prefixed string, using u64_dyn for the length prefix.
		bool str_lp_u64_dyn(const std::string& v) noexcept
		{
			bool ret = u64_dyn(v.size());
			ret &= raw(const_cast<char*>(v.data()), v.size());
			return ret;
		}

		bool bigint_lp_u64_dyn(Bigint& v) SOUP_EXCAL;

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(const std::string& v) noexcept
		{
			bool ret = mysql_lenenc(v.size());
			ret &= raw(const_cast<char*>(v.data()), v.size());
			return ret;
		}

		// Length-prefixed string, using u8 for the length prefix.
		[[deprecated]] bool str_lp_u8(const std::string& v, const uint8_t max_len = 0xFF) noexcept
		{
			return str_lp<u8_t>(v, max_len);
		}

		// Length-prefixed string, using u16 for the length prefix.
		[[deprecated]] bool str_lp_u16(const std::string& v, const uint16_t max_len = 0xFFFF) noexcept
		{
			return str_lp<u16_t>(v, max_len);
		}

		// Length-prefixed string, using u24 for the length prefix.
		[[deprecated]] bool str_lp_u24(const std::string& v, const uint32_t max_len = 0xFFFFFF) noexcept
		{
			return str_lp<u24_t>(v, max_len);
		}

		// Length-prefixed string, using u32 for the length prefix.
		[[deprecated]] bool str_lp_u32(const std::string& v, const uint32_t max_len = 0xFFFFFFFF) noexcept
		{
			return str_lp<u32_t>(v, max_len);
		}

		// Length-prefixed string, using u64 for the length prefix.
		[[deprecated]] bool str_lp_u64(const std::string& v) noexcept
		{
			return str_lp<u64_t>(v);
		}

		// String with known length.
		bool str(size_t len, const std::string& v) noexcept
		{
			size_t pad = (len - v.size());
			bool ret = raw(const_cast<char*>(v.data()), v.size());
			ret &= skip(pad);
			return ret;
		}

		// String with known length.
		bool str(size_t len, const char* v) noexcept
		{
			return raw(const_cast<char*>(v), len);
		}

		// std::vector<uint8_t> with u8 size prefix.
		bool vec_u8_u8(std::vector<uint8_t>& v) noexcept
		{
			SOUP_IF_UNLIKELY (v.size() > 0xFF)
			{
				return false;
			}
			bool ret = true;
			auto len = (uint8_t)v.size();
			ret &= u8(len);
			for (auto& entry : v)
			{
				ret &= u8(entry);
			}
			return ret;
		}

		// std::vector<uint16_t> with u16 size prefix.
		bool vec_u16_u16(std::vector<uint16_t>& v) noexcept
		{
			SOUP_IF_UNLIKELY (v.size() > 0xFFFF)
			{
				return false;
			}
			bool ret = true;
			auto len = (uint16_t)v.size();
			ret &= ioBase::u16(len);
			for (auto& entry : v)
			{
				ret &= ioBase::u16(entry);
			}
			return ret;
		}

		// std::vector<uint16_t> with u16 byte length prefix.
		bool vec_u16_bl_u16(std::vector<uint16_t>& v) noexcept
		{
			size_t bl = (v.size() * sizeof(uint16_t));
			SOUP_IF_UNLIKELY (bl > 0xFFFF)
			{
				return false;
			}
			bool ret = true;
			auto bl_u16 = (uint16_t)bl;
			ret &= ioBase::u16(bl_u16);
			for (auto& entry : v)
			{
				ret &= ioBase::u16(entry);
			}
			return ret;
		}

		// vector of str_nt with u64_dyn length prefix.
		bool vec_str_nt_u64_dyn(std::vector<std::string>& v) noexcept
		{
			bool ret = true;
			uint64_t len = v.size();
			ret &= u64_dyn(len);
			for (auto& entry : v)
			{
				ret &= str_nt(entry);
			}
			return ret;
		}

		// vector of str_lp<u24_t> with u24 byte length prefix.
		bool vec_str_lp_u24_bl_u24(std::vector<std::string>& v) noexcept
		{
			size_t bl = (v.size() * 3);
			for (const auto& entry : v)
			{
				bl += entry.size();
			}
			SOUP_IF_UNLIKELY (bl > 0xFFFFFF)
			{
				return false;
			}
			bool ret = true;
			auto bl_u32 = (uint32_t)bl;
			ret &= ioBase::u24(bl_u32);
			for (auto& entry : v)
			{
				ret &= str_lp<u24_t>(entry);
			}
			return ret;
		}
	};
}
