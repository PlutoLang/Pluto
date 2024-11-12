#pragma once

#include "memAllocator.hpp"

NAMESPACE_SOUP
{
	extern memAllocator g_allocator;

	[[nodiscard]] inline void* malloc(size_t size) SOUP_EXCAL { return g_allocator.allocate(size); }
	[[nodiscard]] inline void* realloc(void* addr, size_t new_size) SOUP_EXCAL { return g_allocator.reallocate(addr, new_size); }
	inline void free(void* addr) noexcept { g_allocator.deallocate(addr); }
}
