#pragma once

#include "base.hpp"

#include <cstddef> // size_t

NAMESPACE_SOUP
{
	class memAllocator
	{
	protected:
		using allocate_impl_t = void*(*)(size_t, void*) /* SOUP_EXCAL */;
		using reallocate_impl_t = void*(*)(void*, size_t, void*) /* SOUP_EXCAL */;
		using deallocate_impl_t = void(*)(void*, void*) /* noexcept */;

		allocate_impl_t allocate_impl;
		reallocate_impl_t reallocate_impl;
		deallocate_impl_t deallocate_impl;
		void* user_data;

		explicit constexpr memAllocator(allocate_impl_t allocate_impl, reallocate_impl_t reallocate_impl, deallocate_impl_t deallocate_impl, void* user_data = nullptr) noexcept
			: allocate_impl(allocate_impl), reallocate_impl(reallocate_impl), deallocate_impl(deallocate_impl), user_data(user_data)
		{
		}

	public:
		[[nodiscard]] void* allocate(size_t size) SOUP_EXCAL
		{
			return allocate_impl(size, user_data);
		}

		void* reallocate(void* addr, size_t new_size) SOUP_EXCAL
		{
			return reallocate_impl(addr, new_size, user_data);
		}

		void deallocate(void* addr) noexcept
		{
			return deallocate_impl(addr, user_data);
		}
	};
}
