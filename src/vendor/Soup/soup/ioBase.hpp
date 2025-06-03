#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Endian.hpp"
#include "IntStruct.hpp"

NAMESPACE_SOUP
{
	// The following types can be used with ioBase::ser<>.
	// For u16 and up, they specify if they are serialised as big endian or little endian. The in-memory format is still expected to be native endian.
	using u8_t = uint8_t;
	SOUP_INT_STRUCT(u16_be_t, uint16_t);
	SOUP_INT_STRUCT(u24_be_t, uint32_t);
	SOUP_INT_STRUCT(u32_be_t, uint32_t);
	SOUP_INT_STRUCT(u16_le_t, uint16_t);
	SOUP_INT_STRUCT(u24_le_t, uint32_t);
	SOUP_INT_STRUCT(u32_le_t, uint32_t);

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

		[[deprecated("Renamed to u16_be")]] bool u16be(uint16_t& v) noexcept { return u16_be(v); }
		[[deprecated("Renamed to u16_le")]] bool u16le(uint16_t& v) noexcept { return u16_le(v); }

		bool u16_be(uint16_t& v) noexcept
		{
			return u16<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u16_le(uint16_t& v) noexcept
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
		[[deprecated("Renamed to u32_be")]] bool u32be(uint32_t& v) noexcept { return u32_be(v); }
		[[deprecated("Renamed to u32_le")]] bool u32le(uint32_t& v) noexcept { return u32_le(v); }

		bool u32_be(uint32_t& v) noexcept
		{
			return u32<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u32_le(uint32_t& v) noexcept
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
		[[deprecated("Renamed to u64_be")]] bool u64be(uint64_t& v) noexcept { return u64_be(v); }
		[[deprecated("Renamed to u64_le")]] bool u64le(uint64_t& v) noexcept { return u64_le(v); }

		bool u64_be(uint64_t& v) noexcept
		{
			return u64<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u64_le(uint64_t& v) noexcept
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

		[[deprecated("Renamed to i16_be")]] bool i16le(int16_t& v) noexcept { return i16_be(v); }
		[[deprecated("Renamed to i16_le")]] bool i16be(int16_t& v) noexcept { return i16_le(v); }

		bool i16(int16_t& v) noexcept { return u16(*(uint16_t*)&v); }
		bool i16_le(int16_t& v) noexcept { return u16_le(*(uint16_t*)&v); }
		bool i16_be(int16_t& v) noexcept { return u16_be(*(uint16_t*)&v); }

		[[deprecated("Renamed to i32_be")]] bool i32le(int32_t& v) noexcept { return i32_be(v); }
		[[deprecated("Renamed to i32_le")]] bool i32be(int32_t& v) noexcept { return i32_le(v); }

		bool i32(int32_t& v) noexcept { return u32(*(uint32_t*)&v); }
		bool i32_le(int32_t& v) noexcept { return u32_le(*(uint32_t*)&v); }
		bool i32_be(int32_t& v) noexcept { return u32_be(*(uint32_t*)&v); }

		[[deprecated("Renamed to i64_be")]] bool i64le(int64_t& v) noexcept { return i64_be(v); }
		[[deprecated("Renamed to i64_le")]] bool i64be(int64_t& v) noexcept { return i64_le(v); }

		bool i64(int64_t& v) noexcept { return u64(*(uint64_t*)&v); }
		bool i64_le(int64_t& v) noexcept { return u64_le(*(uint64_t*)&v); }
		bool i64_be(int64_t& v) noexcept { return u64_be(*(uint64_t*)&v); }

		[[deprecated("Renamed to u24_be")]] bool u24be(uint32_t& v) noexcept { return u24_be(v); }
		[[deprecated("Renamed to u24_le")]] bool u24le(uint32_t& v) noexcept { return u24_le(v); }

		bool u24_be(uint32_t& v) noexcept
		{
			return u24<ENDIAN_NATIVE == ENDIAN_BIG>(v);
		}

		bool u24_le(uint32_t& v) noexcept
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
			return u32_le(*reinterpret_cast<uint32_t*>(&v));
		}

		bool f64(double& v) noexcept
		{
			return u64_le(*reinterpret_cast<uint64_t*>(&v));
		}
	};

#define IOBASE_SER_METHOD_IMPL(t) IOBASE_SER_METHOD_IMPL_2(t, true) IOBASE_SER_METHOD_IMPL_2(t, false)
#define IOBASE_SER_METHOD_IMPL_2(t, is_read) template<> template<> inline bool ioBase<is_read>::ser<t ## _t>(t ## _t& v) noexcept { return t(v); }

	IOBASE_SER_METHOD_IMPL(u8)
	IOBASE_SER_METHOD_IMPL(u16_be)
	IOBASE_SER_METHOD_IMPL(u24_be)
	IOBASE_SER_METHOD_IMPL(u32_be)
	IOBASE_SER_METHOD_IMPL(u16_le)
	IOBASE_SER_METHOD_IMPL(u24_le)
	IOBASE_SER_METHOD_IMPL(u32_le)
}
