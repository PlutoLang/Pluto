#pragma once

#include <cstdint>

#include "Writer.hpp"

NAMESPACE_SOUP
{
	class BitWriter
	{
	public:
		Writer* w;
		uint16_t buffer = 0;
		uint8_t bit_idx = 0;

		BitWriter(Writer* w)
			: w(w)
		{
		}

		[[nodiscard]] Writer& getStream() const noexcept
		{
			return *w;
		}

		[[nodiscard]] constexpr bool isByteAligned() const noexcept
		{
			return bit_idx == 0;
		}

		bool finishByte()
		{
			uint8_t byte = buffer & 0xff;
			buffer = 0;
			bit_idx = 0;
			return w->u8(byte);
		}

	protected:
		bool commitByte()
		{
			uint8_t byte = buffer & 0xff;
			buffer >>= 8;
			bit_idx -= 8;
			return w->u8(byte);
		}

	public:
		template <typename T>
		bool t(uint8_t bits, T val)
		{
			while (bits > 8)
			{
				if (!u8(bits, val & 0xff))
				{
					return false;
				}
				val >>= 8;
				bits -= 8;
			}
			return u8(bits, static_cast<uint8_t>(val));
		}

		bool b(bool val)
		{
			buffer |= (val << bit_idx);
			++bit_idx;
			return bit_idx != 8 || commitByte();
		}

		bool u8(uint8_t bits, uint8_t val)
		{
			if (bits > 8)
			{
				bits = 8;
			}
			val &= ((1 << bits) - 1);

			buffer |= (static_cast<uint16_t>(val) << bit_idx);
			bit_idx += bits;
			return bit_idx < 8 || commitByte();
		}
	};
}
