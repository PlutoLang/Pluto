#pragma once

#include <cstring> // memset

#include "ioBase.hpp"

#include "fwd.hpp"

NAMESPACE_SOUP
{
	class Reader : public ioBase<true>
	{
	public:
		using ioBase::ioBase;

		[[nodiscard]] virtual bool hasMore() noexcept = 0;
		virtual void seek(size_t pos) = 0;
		void seekBegin() { seek(0); }
		virtual void seekEnd() = 0;

		[[nodiscard]] size_t getRemainingBytes()
		{
			const size_t pos = getPosition();
			seekEnd();
			const size_t remaining = (getPosition() - pos);
			seek(pos);
			return remaining;
		}

		bool skip(size_t len)
		{
			seek(getPosition() + len);
			return true;
		}

		virtual const void* getMemoryView(size_t size) const noexcept { return nullptr; }

		// An unsigned 64-bit integer encoded in 1..9 bytes. The most significant bit of bytes 1 to 8 is used to indicate if another byte follows.
		// https://github.com/calamity-inc/u64_dyn
		bool u64_dyn(uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes. (Specialisation of u64_dyn.)
		// https://github.com/calamity-inc/u64_dyn
		bool i64_dyn(int64_t& v) noexcept;

		// An unsigned 64-bit integer encoded in 1..9 bytes. This is a slightly more efficient version of u64_dyn, e.g. 0x4000..0x407f are encoded in 2 bytes instead of 3.
		// https://github.com/calamity-inc/u64_dyn
		bool u64_dyn_v2(uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes. (Specialisation of u64_dyn_v2. This revision also simplifies how negative integers are handled.)
		// https://github.com/calamity-inc/u64_dyn
		bool i64_dyn_v2(int64_t& v) noexcept;

		// An integer where every byte's most significant bit is used to indicate if another byte follows, most significant byte first.
		template <typename Int>
		bool om(Int& v) noexcept
		{
			v = {};
			uint8_t byte;
			while (u8(byte))
			{
				v <<= 7;
				v |= (byte & 0x7F);
				if (!(byte & 0x80))
				{
					return true;
				}
			}
			return false;
		}

		// An integer where every byte's most significant bit is used to indicate if another byte follows, least significant byte first. This is compatible with unsigned LEB128.
		template <typename Int>
		bool oml(Int& v) noexcept
		{
			v = {};
			uint8_t byte;
			uint8_t shift = 0;
			while (u8(byte))
			{
				v |= (static_cast<Int>(byte & 0x7F) << shift);
				if (!(byte & 0x80))
				{
					return true;
				}
				shift += 7;
			}
			return false;
		}

		// Signed LEB128.
		template <typename Int>
		bool soml(Int& v) noexcept
		{
			v = {};
			uint8_t byte;
			uint8_t shift = 0;
			while (u8(byte))
			{
				v |= (static_cast<Int>(byte & 0x7F) << shift);
				if (!(byte & 0x80))
				{
					if (shift < (sizeof(Int) * 8) && (byte & 0x40))
					{
						v |= (~0 << shift);
					}
					return true;
				}
				shift += 7;
			}
			return false;
		}

		bool mysql_lenenc(uint64_t& v) noexcept
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
					if (u16_le(val))
					{
						v = val;
						return true;
					}
				}
				if (first == 0xFD)
				{
					uint32_t val;
					if (u24_le(val))
					{
						v = val;
						return true;
					}
				}
				if (first == 0xFE)
				{
					uint64_t val;
					if (u64_le(val))
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
		bool str_nt(std::string& v) SOUP_EXCAL
		{
			v.clear();
			while (true)
			{
				char c;
				SOUP_RETHROW_FALSE(ioBase::c(c));
				if (c == 0)
				{
					break;
				}
				v.push_back(c);
			}
			return true;
		}

		// Length-prefixed string.
		template <typename T>
		bool str_lp(std::string& v, const T max_len = -1) SOUP_EXCAL
		{
			T len;
			return ser<T>(len) && len <= max_len && str(static_cast<size_t>(len), v);
		}

		// Length-prefixed string, using u64_dyn for the length prefix.
		bool str_lp_u64_dyn(std::string& v) SOUP_EXCAL
		{
			uint64_t len;
			return u64_dyn(len) && str((size_t)len, v);
		}

		// Length-prefixed string, using u64_dyn_v2 for the length prefix.
		bool str_lp_u64_dyn_v2(std::string& v) SOUP_EXCAL
		{
			uint64_t len;
			return u64_dyn_v2(len) && str((size_t)len, v);
		}

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(std::string& v)
		{
			uint64_t len;
			return mysql_lenenc(len) && str((size_t)len, v);
		}

		// String with known length.
		bool str(size_t len, std::string& v) SOUP_EXCAL
		{
			v = std::string(len, '\0');
			return raw(v.data(), len);
		}

		// String with known length.
		bool str(size_t len, char* v) SOUP_EXCAL
		{
			memset(v, 0, len);
			return raw(v, len);
		}

		// std::vector<uint8_t> with u8 size prefix.
		bool vec_u8_u8(std::vector<uint8_t>& v) SOUP_EXCAL
		{
			uint8_t len;
			SOUP_RETHROW_FALSE(u8(len));
			v.clear();
			v.reserve(len);
			while (len--)
			{
				uint8_t entry;
				SOUP_RETHROW_FALSE(u8(entry));
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of u16 with u16 byte length prefix, using big endian over-the-wire.
		bool vec_u16_bl_u16_be(std::vector<uint16_t>& v) SOUP_EXCAL
		{
			uint16_t len;
			SOUP_RETHROW_FALSE(ioBase::u16_be(len));
			v.clear();
			v.reserve(len / 2);
			for (; len >= sizeof(uint16_t); len -= sizeof(uint16_t))
			{
				uint16_t entry;
				SOUP_RETHROW_FALSE(ioBase::u16_be(entry));
				v.emplace_back(entry);
			}
			return true;
		}

		// vector of str_nt with u64_dyn length prefix.
		bool vec_str_nt_u64_dyn(std::vector<std::string>& v) SOUP_EXCAL
		{
			uint64_t len;
			SOUP_RETHROW_FALSE(u64_dyn(len));
			v.clear();
			v.reserve((size_t)len);
			for (; len != 0; --len)
			{
				std::string entry;
				SOUP_RETHROW_FALSE(str_nt(entry));
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_lp<u24_be_t> with u24_be byte length prefix.
		bool vec_str_lp_u24_bl_u24_be(std::vector<std::string>& v) SOUP_EXCAL
		{
			uint32_t len;
			SOUP_RETHROW_FALSE(ioBase::u24_be(len));
			v.clear();
			while (len >= 3)
			{
				std::string entry;
				SOUP_RETHROW_FALSE(str_lp<u24_be_t>(entry));
				len -= ((uint32_t)entry.size() + 3);
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_lp<u8_t> with u16_be byte length prefix.
		bool vec_str_lp_u8_bl_u16_be(std::vector<std::string>& v) SOUP_EXCAL
		{
			uint16_t len;
			SOUP_RETHROW_FALSE(ioBase::u16_be(len));
			v.clear();
			while (len >= 1)
			{
				std::string entry;
				SOUP_RETHROW_FALSE(str_lp<u8_t>(entry));
				len -= ((uint16_t)entry.size() + 1);
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// Null-terminated vector of str_lp<u8_t>.
		bool vec_nt_str_lp_u8(std::vector<std::string>& v) SOUP_EXCAL
		{
			v.clear();
			while (true)
			{
				std::string entry;
				SOUP_RETHROW_FALSE(str_lp<u8_t>(entry));
				if (entry.empty())
				{
					break;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// Reader-specific
		virtual bool getLine(std::string& line) SOUP_EXCAL
		{
			line.clear();
			char c;
			while (ioBase::c(c))
			{
				if (c == '\n')
				{
					return true;
				}
				line.push_back(c);
			}
			return !line.empty();
		}
	};
}
