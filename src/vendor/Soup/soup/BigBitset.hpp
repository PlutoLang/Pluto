#pragma once

#include <cstdint>
#include <cstring> // memcpy

#include "base.hpp"

NAMESPACE_SOUP
{
#pragma pack(push, 1)
	template <size_t Bytes>
	struct BigBitset
	{
		uint8_t data[Bytes]{};
		
		BigBitset() = default;

		BigBitset(const BigBitset<Bytes>& b)
		{
			memcpy(data, b.data, sizeof(data));
		}

		[[nodiscard]] static BigBitset<Bytes>* at(void* dp) noexcept
		{
			return reinterpret_cast<BigBitset<Bytes>*>(dp);
		}

		[[nodiscard]] static const BigBitset<Bytes>* at(const void* dp) noexcept
		{
			return reinterpret_cast<const BigBitset<Bytes>*>(dp);
		}

		[[nodiscard]] constexpr bool get(const size_t i) const noexcept
		{
			const auto j = (i / 8);
			const auto k = (i % 8);

			return (data[j] >> k) & 1;
		}

		constexpr void set(const size_t i, const bool v) noexcept
		{
			const auto j = (i / 8);
			const auto k = (i % 8);

			const uint8_t mask = (1 << k);

			data[j] &= ~mask;
			data[j] |= (mask * v);
		}

		constexpr void enable(const size_t i) noexcept
		{
			const auto j = (i / 8);
			const auto k = (i % 8);

			data[j] |= (1 << k);
		}

		constexpr void disable(const size_t i) noexcept
		{
			const auto j = (i / 8);
			const auto k = (i % 8);

			data[j] &= ~(1 << k);
		}
	};
#pragma pack(pop)
}
