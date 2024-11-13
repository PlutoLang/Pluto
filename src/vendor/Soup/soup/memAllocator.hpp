#pragma once

#include "base.hpp"

#include <cstddef> // size_t

NAMESPACE_SOUP
{
	class memAllocator
	{
	protected:
		using allocate_impl_t = void*(*)(memAllocator*, size_t) /* SOUP_EXCAL */;
		using reallocate_impl_t = void*(*)(memAllocator*, void*, size_t) /* SOUP_EXCAL */;
		using deallocate_impl_t = void(*)(memAllocator*, void*) /* noexcept */;

		allocate_impl_t allocate_impl;
		reallocate_impl_t reallocate_impl;
		deallocate_impl_t deallocate_impl;

		explicit constexpr memAllocator(allocate_impl_t allocate_impl, reallocate_impl_t reallocate_impl, deallocate_impl_t deallocate_impl) noexcept
			: allocate_impl(allocate_impl), reallocate_impl(reallocate_impl), deallocate_impl(deallocate_impl)
		{
		}

	public:
		[[nodiscard]] void* allocate(size_t size) SOUP_EXCAL
		{
			return allocate_impl(this, size);
		}

		void* reallocate(void* addr, size_t new_size) SOUP_EXCAL
		{
			return reallocate_impl(this, addr, new_size);
		}

		void deallocate(void* addr) noexcept
		{
			return deallocate_impl(this, addr);
		}
	};

	extern memAllocator g_default_allocator;
}
