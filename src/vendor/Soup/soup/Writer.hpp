#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

NAMESPACE_SOUP
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
		// https://github.com/calamity-inc/u64_dyn
		bool u64_dyn(const uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes. (Specialisation of u64_dyn.)
		// https://github.com/calamity-inc/u64_dyn
		bool i64_dyn(const int64_t& v) noexcept;

		// An unsigned 64-bit integer encoded in 1..9 bytes. This is a slightly more efficient version of u64_dyn, e.g. 0x4000..0x407f are encoded in 2 bytes instead of 3.
		// https://github.com/calamity-inc/u64_dyn
		bool u64_dyn_v2(const uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes. (Specialisation of u64_dyn_v2. This revision also simplifies how negative integers are handled.)
		// https://github.com/calamity-inc/u64_dyn
		bool i64_dyn_v2(const int64_t& v) noexcept;

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

		// Length-prefixed string, using u64_dyn_v2 for the length prefix.
		bool str_lp_u64_dyn_v2(const std::string& v) noexcept
		{
			bool ret = u64_dyn_v2(v.size());
			ret &= raw(const_cast<char*>(v.data()), v.size());
			return ret;
		}

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(const std::string& v) noexcept
		{
			bool ret = mysql_lenenc(v.size());
			ret &= raw(const_cast<char*>(v.data()), v.size());
			return ret;
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
			SOUP_RETHROW_FALSE(v.size() <= 0xFF);
			bool ret = true;
			auto len = (uint8_t)v.size();
			ret &= u8(len);
			for (auto& entry : v)
			{
				ret &= u8(entry);
			}
			return ret;
		}

		// vector of u16 with u16 byte length prefix, using big endian over-the-wire.
		bool vec_u16_bl_u16_be(std::vector<uint16_t>& v) noexcept
		{
			size_t bl = (v.size() * sizeof(uint16_t));
			SOUP_RETHROW_FALSE(bl <= 0xFFFF);
			bool ret = true;
			auto bl_u16 = (uint16_t)bl;
			ret &= ioBase::u16_be(bl_u16);
			for (auto& entry : v)
			{
				ret &= ioBase::u16_be(entry);
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

		// vector of str_lp<u24_be_t> with u24_be byte length prefix.
		bool vec_str_lp_u24_bl_u24_be(std::vector<std::string>& v) noexcept
		{
			size_t bl = (v.size() * 3);
			for (const auto& entry : v)
			{
				bl += entry.size();
			}
			SOUP_RETHROW_FALSE(bl <= 0xFFFFFF);
			bool ret = true;
			auto bl_u32 = (uint32_t)bl;
			ret &= ioBase::u24_be(bl_u32);
			for (auto& entry : v)
			{
				ret &= str_lp<u24_be_t>(entry);
			}
			return ret;
		}

		// vector of str_lp<u8_t> with u16_be byte length prefix.
		bool vec_str_lp_u8_bl_u16_be(std::vector<std::string>& v) SOUP_EXCAL
		{
			size_t bl = v.size();
			for (const auto& entry : v)
			{
				bl += entry.size();
			}
			SOUP_RETHROW_FALSE(bl <= 0xFFFF);
			bool ret = true;
			auto bl_u16 = (uint16_t)bl;
			ret &= ioBase::u16_be(bl_u16);
			for (auto& entry : v)
			{
				ret &= str_lp<u8_t>(entry);
			}
			return ret;
		}
	};
}
