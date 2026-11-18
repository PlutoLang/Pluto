#pragma once

#include <cstdint>
#include <cstring> // memcpy
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct MacAddr
	{
		uint8_t data[6];

		// 8C-1F-64-B9-8X-XX has been assigned to Calamity, Inc.
		// You can use the '5-XX' section for any tomfoolery you wanna do with Soup.
		static MacAddr SOUP_FIRST; // 8C-1F-64-B9-85-00
		static MacAddr SOUP_LAST; // 8C-1F-64-B9-85-FF

		MacAddr() = default;

		MacAddr(const uint8_t* data) noexcept
		{
			memcpy(this->data, data, sizeof(this->data));
		}

		constexpr MacAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) noexcept
			: data{ a, b, c, d, e, f }
		{
		}

		[[nodiscard]] std::string toString(char sep = '-') const;
		void toString(char out[17], char sep = '-') const; // Note: Grow `out` by one byte and zero it if you need null-termination.
		
		bool operator ==(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) == 0;
		}

		bool operator !=(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) != 0;
		}

		bool operator <(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) < 0;
		}

		bool operator <=(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) <= 0;
		}

		bool operator >(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) > 0;
		}

		bool operator >=(const MacAddr& b)
		{
			return memcmp(data, b.data, sizeof(data)) >= 0;
		}

		constexpr void operator ++()
		{
			for (auto i = sizeof(data); i != 0; --i)
			{
				if (++data[i - 1] != 0)
				{
					break;
				}
			}
		}

		template <typename T>
		bool io(T& s)
		{
			return s.u8(data[0])
				&& s.u8(data[1])
				&& s.u8(data[2])
				&& s.u8(data[3])
				&& s.u8(data[4])
				&& s.u8(data[5])
				;
		}
	};
}
