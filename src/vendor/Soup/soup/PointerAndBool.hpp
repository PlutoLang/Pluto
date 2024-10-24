#pragma once

#include "base.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	template <typename T, SOUP_RESTRICT(std::is_pointer_v<T>)>
	class PointerAndBool
	{
	private:
		uintptr_t data;

	public:
		PointerAndBool(T ptr)
			: data(reinterpret_cast<uintptr_t>(ptr))
		{
			//SOUP_ASSERT((data & 1) == 0);
		}

		PointerAndBool(T ptr, bool b)
			: data(reinterpret_cast<uintptr_t>(ptr))
		{
			//SOUP_ASSERT((data & 1) == 0);
			data |= (uintptr_t)b;
		}

		[[nodiscard]] T getPointer() const noexcept
		{
			return reinterpret_cast<T>(data & ~(uintptr_t)1);
		}

		[[nodiscard]] bool getBool() const noexcept
		{
			return data & 1;
		}

		void setBool(bool b) noexcept
		{
			data &= ~static_cast<uintptr_t>(1);
			data |= static_cast<uintptr_t>(b);
		}

		void set(T ptr, bool b)
		{
			data = reinterpret_cast<uintptr_t>(ptr);
			//SOUP_ASSERT((data & 1) == 0);
			data |= static_cast<uintptr_t>(b);
		}

		operator T() const noexcept
		{
			return getPointer();
		}

		[[nodiscard]] std::remove_pointer_t<T>& operator*() const noexcept
		{
			return *getPointer();
		}

		[[nodiscard]] T operator->() const noexcept
		{
			return getPointer();
		}

		[[nodiscard]] bool operator==(T b) const noexcept
		{
			return getPointer() == b;
		}

		[[nodiscard]] bool operator!=(T b) const noexcept
		{
			return !operator==(b);
		}
	};
}
