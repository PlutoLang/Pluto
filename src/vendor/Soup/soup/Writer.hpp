#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

#include <cstring> // strlen

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

		// Variable-length 64-bit integer codings that take at most 9 bytes. (https://github.com/calamity-inc/u64_dyn)
		bool u64_dyn(const uint64_t& v) noexcept;
		bool u64_dyn_b(const uint64_t& v) noexcept;
		bool u64_dyn_p(const uint64_t& v) noexcept;
		bool u64_dyn_bp(const uint64_t& v) noexcept;
		bool i64_dyn_a(const int64_t& v) noexcept;
		bool i64_dyn_b(const int64_t& v) noexcept;
		bool i64_dyn_p(const int64_t& v) noexcept;
		bool i64_dyn_bp(const int64_t& v) noexcept;
		[[deprecated("Renamed to i64_dyn_a")]] bool i64_dyn(const int64_t& v) noexcept { return i64_dyn_a(v); }
		[[deprecated("Renamed to u64_dyn_b")]] bool u64_dyn_v2(const uint64_t& v) noexcept { return u64_dyn_b(v); }
		[[deprecated("Renamed to i64_dyn_b")]] bool i64_dyn_v2(const int64_t& v) noexcept { return i64_dyn_b(v); }

		template <typename Int>
		[[deprecated("Renamed to omb")]] bool om(const Int& v) noexcept { return omb(v); }

		// An integer where every byte's most significant bit is used to indicate if another byte follows, most significant byte first.
		template <typename Int>
		bool omb(const Int& v) noexcept
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
				if constexpr (!std::is_signed_v<Int>)
				{
					in |= (~static_cast<Int>(0) << ((sizeof(Int) * 8) - 7));
				}
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
		bool str_nt(const char* v)
		{
			return raw(const_cast<char*>(v), strlen(v) + 1);
		}

		// Null-terminated string.
		bool str_nt(const std::string& v) noexcept
		{
			return raw(const_cast<char*>(v.c_str()), v.size() + 1);
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

		// Length-prefixed string, using u64_dyn_b for the length prefix.
		bool str_lp_u64_dyn_b(const std::string& v) noexcept
		{
			bool ret = u64_dyn_b(v.size());
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
			SOUP_RETHROW_FALSE(len >= v.size());
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
