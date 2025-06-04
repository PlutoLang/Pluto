#pragma once

#include <cstddef> // ptrdiff_t
#include <type_traits>

#include "base.hpp"

NAMESPACE_SOUP
{
	template <typename This>
	class Pointerlike
	{
	private:
		[[nodiscard]] void* _addr() const noexcept
		{
			return static_cast<const This*>(this)->addr();
		}

		[[nodiscard]] constexpr uintptr_t _offset() const noexcept
		{
			return static_cast<const This*>(this)->offset();
		}

	public:
		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_pointer_v<T>, T> as() const noexcept
		{
			return reinterpret_cast<T>(_addr());
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_lvalue_reference_v<T>, T> as() const noexcept
		{
			return *static_cast<std::add_pointer_t<std::remove_reference_t<T>>>(_addr());
		}

		template <typename T>
		[[nodiscard]] std::enable_if_t<std::is_same_v<T, uintptr_t>, T> as() const noexcept
		{
			return reinterpret_cast<uintptr_t>(_addr());
		}

		template <typename T>
		[[nodiscard]] constexpr std::enable_if_t<!std::is_pointer_v<T>, T> get() const noexcept
		{
			return *as<T*>();
		}

		template <typename T>
		[[nodiscard]] constexpr std::enable_if_t<std::is_pointer_v<T> && std::is_const_v<std::remove_pointer_t<T>>, T> get() const noexcept
		{
			return as<T>();
		}

		template <typename T>
		void set(T val) const noexcept
		{
			*as<T*>() = val;
		}

		[[nodiscard]] constexpr This add(uintptr_t offset) const noexcept
		{
			return This(_offset() + offset);
		}

		[[nodiscard]] constexpr This sub(uintptr_t offset) const noexcept
		{
			return This(_offset() - offset);
		}

		[[nodiscard]] constexpr This at(ptrdiff_t offset) const noexcept
		{
			return This(_offset() + offset);
		}
	};
}
