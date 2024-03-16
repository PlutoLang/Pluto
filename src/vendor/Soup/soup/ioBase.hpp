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
			: native_endianness(ENDIAN_NATIVE == endian)
		{
		}

		ioVirtualBase(bool little_endian) noexcept
			: ioVirtualBase(little_endian ? ENDIAN_LITTLE : ENDIAN_BIG)
		{
		}

	public:
		[[nodiscard]] bool isBigEndian() const noexcept
		{
			return (ENDIAN_BIG == ENDIAN_NATIVE) == native_endianness;
		}

		[[nodiscard]] bool isLittleEndian() const noexcept
		{
			return (ENDIAN_LITTLE == ENDIAN_NATIVE) == native_endianness;
		}

		virtual ~ioVirtualBase() = default;

		virtual bool raw(void* data, size_t len) noexcept = 0;

		bool b(bool& v) noexcept
		{
			return u8(*(uint8_t*)&v);
		}

		bool c(char& v) noexcept
		{
			return u8(*(uint8_t*)&v);
		}

		bool u8(uint8_t& v) noexcept
		{
			return raw(&v, sizeof(v));
		}
	};

	template <bool is_read>
	class ioBase : public ioVirtualBase
	{
	protected:
		using ioVirtualBase::ioVirtualBase;

	public:
		[[nodiscard]] static constexpr bool isRead() noexcept
		{
			return is_read;
		}

		[[nodiscard]] static constexpr bool isWrite() noexcept
		{
			return !isRead();
		}

		template <typename T>
		bool ser(T& v) noexcept;

		bool u16(uint16_t& v) noexcept
		{
			return u16(v, native_endianness);
		}

		bool u16_be(uint16_t& v) noexcept
		{
			return u16(v, ENDIAN_NATIVE == ENDIAN_BIG);
		}

		bool u16_le(uint16_t& v) noexcept
		{
			return u16(v, ENDIAN_NATIVE == ENDIAN_LITTLE);
		}

	protected:
		bool u16(uint16_t& v, bool native_endianness) noexcept
		{
			if (native_endianness)
			{
				return raw(&v, sizeof(v));
			}
			else
			{
				if constexpr (is_read)
				{
					return raw(&v, sizeof(v))
						&& (v = Endianness::invert(v), true)
						;
				}
				else
				{
					auto tmp = Endianness::invert(v);
					return raw(&tmp, sizeof(v));
				}
			}
		}

	public:
		bool u32(uint32_t& v) noexcept
		{
			return u32(v, native_endianness);
		}

		bool u32_be(uint32_t& v) noexcept
		{
			return u32(v, ENDIAN_NATIVE == ENDIAN_BIG);
		}

		bool u32_le(uint32_t& v) noexcept
		{
			return u32(v, ENDIAN_NATIVE == ENDIAN_LITTLE);
		}

	protected:
		bool u32(uint32_t& v, bool native_endianness) noexcept
		{
			if (native_endianness)
			{
				return raw(&v, sizeof(v));
			}
			else
			{
				if constexpr (is_read)
				{
					return raw(&v, sizeof(v))
						&& (v = Endianness::invert(v), true)
						;
				}
				else
				{
					auto tmp = Endianness::invert(v);
					return raw(&tmp, sizeof(v));
				}
			}
		}

	public:
		bool u64(uint64_t& v) noexcept
		{
			return u64(v, native_endianness);
		}

		bool u64_be(uint64_t& v) noexcept
		{
			return u64(v, ENDIAN_NATIVE == ENDIAN_BIG);
		}

		bool u64_le(uint64_t& v) noexcept
		{
			return u64(v, ENDIAN_NATIVE == ENDIAN_LITTLE);
		}

	protected:
		bool u64(uint64_t& v, bool native_endianness) noexcept
		{
			if (native_endianness)
			{
				return raw(&v, sizeof(v));
			}
			else
			{
				if constexpr (is_read)
				{
					return raw(&v, sizeof(v))
						&& (v = Endianness::invert(v), true)
						;
				}
				else
				{
					auto tmp = Endianness::invert(v);
					return raw(&tmp, sizeof(v));
				}
			}
		}

	public:
		bool i8(int8_t& v) noexcept
		{
			return u8(*(uint8_t*)&v);
		}

		bool i16(int16_t& v) noexcept
		{
			return u16(*(uint16_t*)&v);
		}

		bool i32(int32_t& v) noexcept
		{
			return u32(*(uint32_t*)&v);
		}

		bool i64(int64_t& v) noexcept
		{
			return u64(*(uint64_t*)&v);
		}

		bool u24(uint32_t& v) noexcept
		{
			return u24(v, native_endianness);
		}

		bool u24_be(uint32_t& v) noexcept
		{
			return u24(v, ENDIAN_NATIVE == ENDIAN_BIG);
		}

		bool u24_le(uint32_t& v) noexcept
		{
			return u24(v, ENDIAN_NATIVE == ENDIAN_LITTLE);
		}

	protected:
		bool u24(uint32_t& v, bool native_endianness) noexcept
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
		bool u40(uint64_t& v) noexcept
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

		bool u48(uint64_t& v) noexcept
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

		bool u56(uint64_t& v) noexcept
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

		bool f32(float& v) noexcept
		{
			return raw(&v, 4);
		}

		bool f64(double& v) noexcept
		{
			return raw(&v, 8);
		}
	};

#define IOBASE_SER_METHOD_IMPL(t) IOBASE_SER_METHOD_IMPL_2(t, true) IOBASE_SER_METHOD_IMPL_2(t, false)
#define IOBASE_SER_METHOD_IMPL_2(t, is_read) template<> template<> inline bool ioBase<is_read>::ser<t ## _t>(t ## _t& v) noexcept { return t(v); }

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
