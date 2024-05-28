#pragma once

#include "ioBase.hpp"

#include "fwd.hpp"

NAMESPACE_SOUP
{
	class Reader : public ioBase<true>
	{
	public:
		using ioBase::ioBase;

		[[nodiscard]] virtual bool hasMore() noexcept = 0;
		[[nodiscard]] virtual size_t getPosition() = 0;
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

		// An unsigned 64-bit integer encoded in 1..9 bytes. The most significant bit of bytes 1 to 8 is used to indicate if another byte follows.
		bool u64_dyn(uint64_t& v) noexcept;

		// A signed 64-bit integer encoded in 1..9 bytes.
		bool i64_dyn(int64_t& v) noexcept;

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
				SOUP_IF_UNLIKELY (!ioBase::c(c))
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

		// Length-prefixed string, using mysql_lenenc for the length prefix.
		bool str_lp_mysql(std::string& v)
		{
			uint64_t len;
			return mysql_lenenc(len) && str((size_t)len, v);
		}

		// Length-prefixed string, using u8 for the length prefix.
		[[deprecated]] bool str_lp_u8(std::string& v, const uint8_t max_len = 0xFF) SOUP_EXCAL
		{
			return str_lp<u8_t>(v, max_len);
		}

		// Length-prefixed string, using u16 for the length prefix.
		[[deprecated]] bool str_lp_u16(std::string& v, const uint16_t max_len = 0xFFFF) SOUP_EXCAL
		{
			return str_lp<u16_t>(v, max_len);
		}

		// Length-prefixed string, using u24 for the length prefix.
		[[deprecated]] bool str_lp_u24(std::string& v, const uint32_t max_len = 0xFFFFFF) SOUP_EXCAL
		{
			return str_lp<u24_t>(v, max_len);
		}

		// Length-prefixed string, using u32 for the length prefix.
		[[deprecated]] bool str_lp_u32(std::string& v, const uint32_t max_len = 0xFFFFFFFF) SOUP_EXCAL
		{
			return str_lp<u32_t>(v, max_len);
		}

		// Length-prefixed string, using u64 for the length prefix.
		[[deprecated]] bool str_lp_u64(std::string& v) SOUP_EXCAL
		{
			return str_lp<u64_t>(v);
		}

		// String with known length.
		bool str(size_t len, std::string& v) SOUP_EXCAL
		{
			v = std::string(len, '\0');
			return raw(v.data(), len);
		}

		// std::vector<uint8_t> with u8 size prefix.
		bool vec_u8_u8(std::vector<uint8_t>& v) SOUP_EXCAL
		{
			uint8_t len;
			SOUP_IF_UNLIKELY (!u8(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len);
			while (len--)
			{
				uint8_t entry;
				SOUP_IF_UNLIKELY (!u8(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// std::vector<uint16_t> with u16 size prefix.
		bool vec_u16_u16(std::vector<uint16_t>& v) SOUP_EXCAL
		{
			uint16_t len;
			SOUP_IF_UNLIKELY (!ioBase::u16(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 2);
			while (len--)
			{
				uint16_t entry;
				SOUP_IF_UNLIKELY (!ioBase::u16(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// std::vector<uint16_t> with u16 byte length prefix.
		bool vec_u16_bl_u16(std::vector<uint16_t>& v) SOUP_EXCAL
		{
			uint16_t len;
			SOUP_IF_UNLIKELY (!ioBase::u16(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 2);
			for (; len >= sizeof(uint16_t); len -= sizeof(uint16_t))
			{
				uint16_t entry;
				SOUP_IF_UNLIKELY (!ioBase::u16(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_nt with u64_dyn length prefix.
		bool vec_str_nt_u64_dyn(std::vector<std::string>& v) SOUP_EXCAL
		{
			uint64_t len;
			SOUP_IF_UNLIKELY (!u64_dyn(len))
			{
				return false;
			}
			v.clear();
			v.reserve((size_t)len);
			for (; len != 0; --len)
			{
				std::string entry;
				SOUP_IF_UNLIKELY (!str_nt(entry))
				{
					return false;
				}
				v.emplace_back(std::move(entry));
			}
			return true;
		}

		// vector of str_lp<u24_t> with u24 byte length prefix.
		bool vec_str_lp_u24_bl_u24(std::vector<std::string>& v) SOUP_EXCAL
		{
			uint32_t len;
			SOUP_IF_UNLIKELY (!ioBase::u24(len))
			{
				return false;
			}
			v.clear();
			v.reserve(len / 3);
			while (len >= 3)
			{
				std::string entry;
				SOUP_IF_UNLIKELY (!str_lp<u24_t>(entry))
				{
					return false;
				}
				len -= ((uint32_t)entry.size() + 3);
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
				SOUP_IF_UNLIKELY (!str_lp<u8_t>(entry))
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
