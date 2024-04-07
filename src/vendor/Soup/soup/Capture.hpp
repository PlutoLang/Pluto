#pragma once

#include <cstdint> // uintptr_t
#include <utility> // move

#include "base.hpp" // SOUP_EXCAL
#include "deleter.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	class Capture
	{
	protected:
		void* data = nullptr;
		deleter_t deleter = nullptr;

	public:
		Capture() noexcept = default;

		Capture(const Capture&) = delete;

		Capture(Capture&& b) noexcept
			: data(b.data), deleter(b.deleter)
		{
#ifdef _DEBUG
			validate();
#endif
			b.forget();
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		Capture(const T& v) SOUP_EXCAL
			: data(new std::remove_reference_t<T>(v)), deleter(&deleter_impl<std::remove_reference_t<T>>)
		{
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		Capture(T&& v) SOUP_EXCAL
			: data(new std::remove_reference_t<T>(std::move(v))), deleter(&deleter_impl<std::remove_reference_t<T>>)
		{
		}

		// For some reason, C++ thinks it can call the T&& overload for non-const SharedPtr references...
		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		Capture(T& v) SOUP_EXCAL
			: data(new std::remove_reference_t<T>(v)), deleter(&deleter_impl<std::remove_reference_t<T>>)
		{
		}

		template <typename T, SOUP_RESTRICT(std::is_pointer_v<std::remove_reference_t<T>>)>
		Capture(T v) noexcept
			: data(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(v)))
		{
#ifdef _DEBUG
			validate();
#endif
		}

		~Capture() noexcept
		{
			free();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return data == nullptr;
		}

		void reset() noexcept
		{
			free();
			forget();
		}

	protected:
		void free() noexcept
		{
			if (deleter != nullptr)
			{
				deleter(data);
			}
		}

		void forget() noexcept
		{
			data = nullptr;
			deleter = nullptr;
		}

	public:
		void operator =(const Capture&) = delete;

		void operator =(Capture&& b) noexcept
		{
			free();
			data = b.data;
			deleter = b.deleter;
			b.forget();
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		void operator =(const T& v) SOUP_EXCAL
		{
			free();
			data = new std::remove_reference_t<T>(v);
			deleter = &deleter_impl<std::remove_reference_t<T>>;
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		void operator =(T&& v) SOUP_EXCAL
		{
			free();
			data = new std::remove_reference_t<T>(std::move(v));
			deleter = &deleter_impl<std::remove_reference_t<T>>;
		}

		template <typename T, SOUP_RESTRICT(std::is_pointer_v<std::remove_reference_t<T>>)>
		void operator =(T v) noexcept
		{
			free();
			data = v;
			deleter = nullptr;
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return data != nullptr;
		}

		template <typename T, SOUP_RESTRICT(!std::is_pointer_v<std::remove_reference_t<T>>)>
		[[nodiscard]] T& get() const noexcept
		{
#ifdef _DEBUG
			validate();
#endif
			return *reinterpret_cast<T*>(data);
		}

		template <typename T, SOUP_RESTRICT(std::is_pointer_v<std::remove_reference_t<T>>)>
		[[nodiscard]] T get() const noexcept
		{
#ifdef _DEBUG
			validate();
#endif
			return reinterpret_cast<T>(data);
		}

	private:
		void validate() const;
	};
}
