#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Endian.hpp"

namespace soup
{
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

		bool u32(uint32_t& v)
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

		bool u64(uint64_t& v)
		{
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
		}

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

		bool u24(uint32_t& v)
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
}
