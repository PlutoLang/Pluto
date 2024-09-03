#pragma once

#include <utility> // forward

#include "base.hpp"
#include "type_traits.hpp"

NAMESPACE_SOUP
{
	template <typename T>
	class UniquePtr
	{
	public:
		T* data = nullptr;

		UniquePtr() noexcept = default;

		UniquePtr(T* ptr) noexcept
			: data(ptr)
		{
		}

		UniquePtr(UniquePtr<T>&& b) noexcept
			: data(b.data)
		{
			b.data = nullptr;
		}

		template <typename T2, SOUP_RESTRICT(std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>)>
		UniquePtr(UniquePtr<T2>&& b) noexcept
			: data(static_cast<T*>(b.data))
		{
			b.data = nullptr;
		}

		~UniquePtr()
		{
			free();
		}

	private:
		void free()
		{
			// "if ptr is a null pointer, the standard library deallocation functions do nothing"
			//if (data != nullptr)
			{
				delete data;
			}
		}

	public:
		void reset() noexcept
		{
			// "if ptr is a null pointer, the standard library deallocation functions do nothing"
			//if (data != nullptr)
			{
				delete data;
				data = nullptr;
			}
		}

		UniquePtr<T>& operator =(UniquePtr<T>&& b) noexcept
		{
			const auto old_data = data;
			data = b.data;
			b.data = nullptr;
			// "if ptr is a null pointer, the standard library deallocation functions do nothing"
			//if (old_data != nullptr)
			{
				delete old_data;
			}
			return *this;
		}

		[[nodiscard]] operator bool() const noexcept
		{
			return data != nullptr;
		}

		[[nodiscard]] operator T*() const noexcept
		{
			return get();
		}

		[[nodiscard]] T* get() const noexcept
		{
			return data;
		}

		[[nodiscard]] T& operator*() const noexcept
		{
			return *get();
		}

		[[nodiscard]] T* operator->() const noexcept
		{
			return get();
		}

		[[nodiscard]] T* release() noexcept
		{
			T* val = data;
			data = nullptr;
			return val;
		}
	};

	template <typename T, typename...Args, SOUP_RESTRICT(!std::is_array_v<T>)>
	[[nodiscard]] UniquePtr<T> make_unique(Args&&...args)
	{
		return UniquePtr<T>(new T(std::forward<Args>(args)...));
	}
}
