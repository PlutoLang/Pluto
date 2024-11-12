#include "alloc.hpp"

#include "memCAllocator.hpp"

NAMESPACE_SOUP
{
	memAllocator g_allocator = memCAllocator();
}
