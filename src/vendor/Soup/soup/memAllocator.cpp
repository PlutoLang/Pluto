#include "memAllocator.hpp"

#include "memCAllocator.hpp"

NAMESPACE_SOUP
{
	memAllocator g_default_allocator = memCAllocator();
}
