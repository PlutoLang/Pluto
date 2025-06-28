#pragma once

#include "base.hpp"

#include "alloc.hpp"

NAMESPACE_SOUP
{
	template <typename AllocatorT>
	struct memAllocating
	{
		AllocatorT& allocator;

		memAllocating(AllocatorT& allocator)
			: allocator(allocator)
		{
		}

		[[nodiscard]] void* allocate(size_t size) const SOUP_EXCAL
		{
			return allocator.allocate(size);
		}

		[[nodiscard]] void* reallocate(void* addr, size_t new_size) const SOUP_EXCAL
		{
			return allocator.reallocate(addr, new_size);
		}

		void deallocate(void* addr) const noexcept
		{
			return allocator.deallocate(addr);
		}
	};

	template <>
	struct memAllocating<void>
	{
		[[nodiscard]] void* allocate(size_t size) const SOUP_EXCAL
		{
			return soup::malloc(size);
		}

		[[nodiscard]] void* reallocate(void* addr, size_t new_size) const SOUP_EXCAL
		{
			return soup::realloc(addr, new_size);
		}

		void deallocate(void* addr) const noexcept
		{
			return soup::free(addr);
		}
	};
}
