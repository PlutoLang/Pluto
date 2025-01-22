#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Endian.hpp"
#include "IntStruct.hpp"

NAMESPACE_SOUP
{
	using u8_t = uint8_t;
	using u16_t = uint16_t;
	using u32_t = uint32_t;
	using u64_t = uint64_t;

	SOUP_INT_STRUCT(u24_t, u32_t);
	SOUP_INT_STRUCT(u40_t, u64_t);
	SOUP_INT_STRUCT(u48_t, u64_t);
	SOUP_INT_STRUCT(u56_t, u64_t);

	SOUP_INT_STRUCT(u16be_t, u16_t);
	SOUP_INT_STRUCT(u24be_t, u32_t);
	SOUP_INT_STRUCT(u32be_t, u32_t);

	SOUP_INT_STRUCT(u16le_t, u16_t);
	SOUP_INT_STRUCT(u24le_t, u32_t);
	SOUP_INT_STRUCT(u32le_t, u32_t);

	class ioVirtualBase
	{
	protected:
		ioVirtualBase() noexcept
		{
		}

	public:
		virtual ~ioVirtualBase() = default;

		virtual bool raw(void* data, size_t len) noexcept = 0;
		[[nodiscard]] virtual size_t getPosition() = 0;

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

		[[deprecated("Renamed to u16be")]] bool u16_be(uint16_t& v) noexcept { return u16be(v); }
		[[deprecated("Renamed to u16le")]] bool u16_le(uint16_t& v) noexcept { return u16le(v); }

		bool u16be(uint16_t& v) noexcept
		{
			return u16<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u16le(uint16_t& v) noexcept
		{
			return u16<ENDIAN_NATIVE == ENDIAN_LITTLE>(v);
		}

	protected:
		template <bool native_endianness>
		bool u16(uint16_t& v) noexcept
		{
			if constexpr (native_endianness)
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
		[[deprecated("Renamed to u32be")]] bool u32_be(uint32_t& v) noexcept { return u32be(v); }
		[[deprecated("Renamed to u32le")]] bool u32_le(uint32_t& v) noexcept { return u32le(v); }

		bool u32be(uint32_t& v) noexcept
		{
			return u32<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u32le(uint32_t& v) noexcept
		{
			return u32<ENDIAN_NATIVE == ENDIAN_LITTLE>(v);
		}

	protected:
		template <bool native_endianness>
		bool u32(uint32_t& v) noexcept
		{
			if constexpr (native_endianness)
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
		[[deprecated("Renamed to u64be")]] bool u64_be(uint64_t& v) noexcept { return u64be(v); }
		[[deprecated("Renamed to u64le")]] bool u64_le(uint64_t& v) noexcept { return u64le(v); }

		bool u64be(uint64_t& v) noexcept
		{
			return u64<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u64le(uint64_t& v) noexcept
		{
			return u64<ENDIAN_NATIVE == ENDIAN_LITTLE>(v);
		}

	protected:
		template <bool native_endianness>
		bool u64(uint64_t& v) noexcept
		{
			if constexpr (native_endianness)
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

		bool i16(int16_t& v) noexcept { return u16(*(uint16_t*)&v); }
		bool i16le(int16_t& v) noexcept { return u16le(*(uint16_t*)&v); }
		bool i16be(int16_t& v) noexcept { return u16be(*(uint16_t*)&v); }

		bool i32(int32_t& v) noexcept { return u32(*(uint32_t*)&v); }
		bool i32le(int32_t& v) noexcept { return u32le(*(uint32_t*)&v); }
		bool i32be(int32_t& v) noexcept { return u32be(*(uint32_t*)&v); }

		bool i64(int64_t& v) noexcept { return u64(*(uint64_t*)&v); }
		bool i64le(int64_t& v) noexcept { return u64le(*(uint64_t*)&v); }
		bool i64be(int64_t& v) noexcept { return u64be(*(uint64_t*)&v); }

		[[deprecated("Renamed to u24be")]] bool u24_be(uint32_t& v) noexcept { return u24be(v); }
		[[deprecated("Renamed to u24le")]] bool u24_le(uint32_t& v) noexcept { return u24le(v); }

		bool u24be(uint32_t& v) noexcept
		{
			return u24<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u24le(uint32_t& v) noexcept
		{
			return u24<ENDIAN_NATIVE == ENDIAN_LITTLE>(v);
		}

	protected:
		template <bool native_endianness>
		bool u24(uint32_t& v) noexcept
		{
			if constexpr (is_read)
			{
				v = 0;
			}
			if constexpr (native_endianness)
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
		bool f32(float& v) noexcept
		{
			return u32le(*reinterpret_cast<uint32_t*>(&v));
		}

		bool f64(double& v) noexcept
		{
			return u64le(*reinterpret_cast<uint64_t*>(&v));
		}
	};

#define IOBASE_SER_METHOD_IMPL(t) IOBASE_SER_METHOD_IMPL_2(t, true) IOBASE_SER_METHOD_IMPL_2(t, false)
#define IOBASE_SER_METHOD_IMPL_2(t, is_read) template<> template<> inline bool ioBase<is_read>::ser<t ## _t>(t ## _t& v) noexcept { return t(v); }

	IOBASE_SER_METHOD_IMPL(u8)

	IOBASE_SER_METHOD_IMPL(u16be)
	IOBASE_SER_METHOD_IMPL(u24be)
	IOBASE_SER_METHOD_IMPL(u32be)

	IOBASE_SER_METHOD_IMPL(u16le)
	IOBASE_SER_METHOD_IMPL(u24le)
	IOBASE_SER_METHOD_IMPL(u32le)
}
