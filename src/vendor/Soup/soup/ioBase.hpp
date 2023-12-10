#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Endian.hpp"
#include "IntStruct.hpp"

namespace soup
{
	using u8_t = uint8_t;
	using u16_t = uint16_t;
	using u32_t = uint32_t;
	using u64_t = uint64_t;

	SOUP_INT_STRUCT(u24_t, u32_t);
	SOUP_INT_STRUCT(u40_t, u64_t);
	SOUP_INT_STRUCT(u48_t, u64_t);
	SOUP_INT_STRUCT(u56_t, u64_t);

	SOUP_INT_STRUCT(u16_be_t, u16_t);
	SOUP_INT_STRUCT(u24_be_t, u32_t);
	SOUP_INT_STRUCT(u32_be_t, u32_t);

	SOUP_INT_STRUCT(u16_le_t, u16_t);
	SOUP_INT_STRUCT(u24_le_t, u32_t);
	SOUP_INT_STRUCT(u32_le_t, u32_t);

	class ioVirtualBase
	{
	protected:
		const bool native_endianness;

		ioVirtualBase(Endian endian) noexcept
			: native_endianness(NATIVE_ENDIAN == endian)
		{
		}

		ioVirtualBase(bool little_endian) noexcept
			: ioVirtualBase(little_endian ? LITTLE_ENDIAN : BIG_ENDIAN)
		{
		}

	public:
		[[nodiscard]] bool isBigEndian() const noexcept
		{
			return (BIG_ENDIAN == NATIVE_ENDIAN) == native_endianness;
		}

		[[nodiscard]] bool isLittleEndian() const noexcept
		{
			return (LITTLE_ENDIAN == NATIVE_ENDIAN) == native_endianness;
		}

		virtual ~ioVirtualBase() = default;

		[[nodiscard]] virtual bool hasMore()
		{
			return true;
		}

		bool b(bool& v)
		{
			return u8(*(uint8_t*)&v);
		}

		bool c(char& v)
		{
			return u8(*(uint8_t*)&v);
		}

		virtual bool u8(uint8_t& v) = 0;

		bool u16(uint16_t& v)
		{
			return u16(v, native_endianness);
		}

		bool u16_be(uint16_t& v)
		{
			return u16(v, NATIVE_ENDIAN == BIG_ENDIAN);
		}

		bool u16_le(uint16_t& v)
		{
			return u16(v, NATIVE_ENDIAN == LITTLE_ENDIAN);
		}

	protected:
		bool u16(uint16_t& v, bool native_endianness)
		{
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1]);
			}
			else
			{
				return u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}

	public:
		bool u32(uint32_t& v)
		{
			return u32(v, native_endianness);
		}

		bool u32_be(uint32_t& v)
		{
			return u32(v, NATIVE_ENDIAN == BIG_ENDIAN);
		}

		bool u32_le(uint32_t& v)
		{
			return u32(v, NATIVE_ENDIAN == LITTLE_ENDIAN);
		}

	protected:
		bool u32(uint32_t& v, bool native_endianness)
		{
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[3]);
			}
			else
			{
				return u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}

	public:
		bool u64(uint64_t& v)
		{
			return u64(v, native_endianness);
		}

		bool u64_be(uint64_t& v)
		{
			return u64(v, NATIVE_ENDIAN == BIG_ENDIAN);
		}

		bool u64_le(uint64_t& v)
		{
			return u64(v, NATIVE_ENDIAN == LITTLE_ENDIAN);
		}

	protected:
		bool u64(uint64_t& v, bool native_endianness)
		{
#if true
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[5])
					&& u8(((uint8_t*)&v)[6])
					&& u8(((uint8_t*)&v)[7]);
			}
			else
			{
				return u8(((uint8_t*)&v)[7])
					&& u8(((uint8_t*)&v)[6])
					&& u8(((uint8_t*)&v)[5])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
#else
			uint8_t tmp;
			if (isLittleEndian())
			{
				return u8((tmp = v >> 0, tmp))
					&& u8((tmp = v >> 8, tmp))
					&& u8((tmp = v >> 16, tmp))
					&& u8((tmp = v >> 24, tmp))
					&& u8((tmp = v >> 32, tmp))
					&& u8((tmp = v >> 40, tmp))
					&& u8((tmp = v >> 48, tmp))
					&& u8((tmp = v >> 56, tmp));
			}
			else
			{
				return u8((tmp = v >> 56, tmp))
					&& u8((tmp = v >> 48, tmp))
					&& u8((tmp = v >> 40, tmp))
					&& u8((tmp = v >> 32, tmp))
					&& u8((tmp = v >> 24, tmp))
					&& u8((tmp = v >> 16, tmp))
					&& u8((tmp = v >> 8, tmp))
					&& u8((tmp = v >> 0, tmp));
			}
#endif
		}

	public:
		bool i8(int8_t& v)
		{
			return u8(*(uint8_t*)&v);
		}

		bool i16(int16_t& v)
		{
			return u16(*(uint16_t*)&v);
		}

		bool i32(int32_t& v)
		{
			return u32(*(uint32_t*)&v);
		}

		bool i64(int64_t& v)
		{
			return u64(*(uint64_t*)&v);
		}
	};

	template <bool is_read>
	class ioBase : public ioVirtualBase
	{
	protected:
		using ioVirtualBase::ioVirtualBase;

	public:
		[[nodiscard]] static constexpr bool isRead()
		{
			return is_read;
		}

		[[nodiscard]] static constexpr bool isWrite()
		{
			return !isRead();
		}

		template <typename T>
		bool ser(T& v);

		bool u24(uint32_t& v)
		{
			return u24(v, native_endianness);
		}

		bool u24_be(uint32_t& v)
		{
			return u24(v, NATIVE_ENDIAN == BIG_ENDIAN);
		}

		bool u24_le(uint32_t& v)
		{
			return u24(v, NATIVE_ENDIAN == LITTLE_ENDIAN);
		}

	protected:
		bool u24(uint32_t& v, bool native_endianness)
		{
			if (isRead())
			{
				v = 0;
			}
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2]);
			}
			else
			{
				return u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}

	public:
		bool u40(uint64_t& v)
		{
			if (isRead())
			{
				v = 0;
			}
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[4]);
			}
			else
			{
				return u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}

		bool u48(uint64_t& v)
		{
			if (isRead())
			{
				v = 0;
			}
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[5]);
			}
			else
			{
				return u8(((uint8_t*)&v)[5])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}

		bool u56(uint64_t& v)
		{
			if (isRead())
			{
				v = 0;
			}
			if (native_endianness)
			{
				return u8(((uint8_t*)&v)[0])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[5])
					&& u8(((uint8_t*)&v)[6]);
			}
			else
			{
				return u8(((uint8_t*)&v)[6])
					&& u8(((uint8_t*)&v)[5])
					&& u8(((uint8_t*)&v)[4])
					&& u8(((uint8_t*)&v)[3])
					&& u8(((uint8_t*)&v)[2])
					&& u8(((uint8_t*)&v)[1])
					&& u8(((uint8_t*)&v)[0]);
			}
		}
	};

#define IOBASE_SER_METHOD_IMPL(t) IOBASE_SER_METHOD_IMPL_2(t, true) IOBASE_SER_METHOD_IMPL_2(t, false)
#define IOBASE_SER_METHOD_IMPL_2(t, is_read) template<> template<> inline bool ioBase<is_read>::ser<t ## _t>(t ## _t& v) { return t(v); }

	IOBASE_SER_METHOD_IMPL(u8)
	IOBASE_SER_METHOD_IMPL(u16)
	IOBASE_SER_METHOD_IMPL(u24)
	IOBASE_SER_METHOD_IMPL(u32)
	IOBASE_SER_METHOD_IMPL(u40)
	IOBASE_SER_METHOD_IMPL(u48)
	IOBASE_SER_METHOD_IMPL(u56)
	IOBASE_SER_METHOD_IMPL(u64)

	IOBASE_SER_METHOD_IMPL(u16_be)
	IOBASE_SER_METHOD_IMPL(u24_be)
	IOBASE_SER_METHOD_IMPL(u32_be)

	IOBASE_SER_METHOD_IMPL(u16_le)
	IOBASE_SER_METHOD_IMPL(u24_le)
	IOBASE_SER_METHOD_IMPL(u32_le)
}
