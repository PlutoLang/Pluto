#pragma once

#include <cstdint>

#include "base.hpp"

NAMESPACE_SOUP
{
	template <typename IntT>
	struct IntStruct
	{
		IntT data;

		constexpr IntStruct(IntT val = 0) noexcept
			: data(val)
		{
		}

		constexpr IntStruct(const IntStruct& b) noexcept
			: data(b.data)
		{
		}

		constexpr void operator =(IntT val) noexcept
		{
			data = val;
		}

		constexpr operator IntT& () noexcept
		{
			return data;
		}

		constexpr operator const IntT& () const noexcept
		{
			return data;
		}
	};
	static_assert(sizeof(IntStruct<uint8_t>) == sizeof(uint8_t));
	static_assert(sizeof(IntStruct<uint16_t>) == sizeof(uint16_t));
	static_assert(sizeof(IntStruct<uint32_t>) == sizeof(uint32_t));
	static_assert(sizeof(IntStruct<uint64_t>) == sizeof(uint64_t));
}

#define SOUP_INT_STRUCT(name, type) struct name : public ::soup::IntStruct<type> { using IntStruct::IntStruct; };
