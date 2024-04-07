#pragma once

#include <cstdint>
#include <utility> // move

#include "base.hpp"

NAMESPACE_SOUP
{
#pragma pack(push, 1)
	template <typename T = uint32_t>
	struct Bitset
	{
		T data;

		constexpr Bitset(T&& data = {}) noexcept
			: data(std::move(data))
		{
		}

		[[nodiscard]] static Bitset<T>& at(T& dp) noexcept
		{
			return *at(&dp);
		}

		[[nodiscard]] static Bitset<T>* at(T* dp) noexcept
		{
			return reinterpret_cast<Bitset<T>*>(dp);
		}

		[[nodiscard]] static const Bitset<T>& at(const T& dp) noexcept
		{
			return *at(&dp);
		}

		[[nodiscard]] static const Bitset<T>* at(const T* dp) noexcept
		{
			return reinterpret_cast<const Bitset<T>*>(dp);
		}

		[[nodiscard]] bool get(const uint8_t i) const noexcept
		{
			return (data >> i) & 1;
		}

		[[nodiscard]] bool getMask(const T mask) const noexcept
		{
			return (data & mask) != 0;
		}

		[[nodiscard]] static T calculateMask(const uint8_t i) noexcept
		{
			return static_cast<T>(1) << i;
		}

		void set(const uint8_t i, const bool v) noexcept
		{
			setMask(calculateMask(i), v);
		}

		void setMask(const T mask, const bool v) noexcept
		{
			data &= ~mask;
			data |= (mask * v);
		}

		void enable(const uint8_t i) noexcept
		{
			enableMask(calculateMask(i));
		}

		void enableMask(const T mask) noexcept
		{
			data |= mask;
		}

		void disable(const uint8_t i) noexcept
		{
			disableMask(calculateMask(i));
		}

		void disableMask(const T mask) noexcept
		{
			data &= ~mask;
		}

		operator T& () noexcept
		{
			return data;
		}
	};
#pragma pack(pop)
}
