#pragma once

#include "memAllocator.hpp"

#include <cstdlib> // malloc, realloc, free
#include <new> // bad_alloc

NAMESPACE_SOUP
{
	class memCAllocator : public memAllocator
	{
	public:
		constexpr memCAllocator() noexcept
			: memAllocator(&allocateImpl, &reallocateImpl, &deallocateImpl)
		{
		}

	protected:
		static void* allocateImpl(size_t size, void*) /* SOUP_EXCAL */
		{
			void* ptr = ::malloc(size);
			SOUP_IF_LIKELY (ptr)
			{
				return ptr;
			}
			SOUP_THROW(std::bad_alloc{});
		}

		static void* reallocateImpl(void* addr, size_t new_size, void*) /* SOUP_EXCAL */
		{
			addr = ::realloc(addr, new_size);
			SOUP_IF_LIKELY (addr)
			{
				return addr;
			}
			SOUP_THROW(std::bad_alloc{});
		}

		static void deallocateImpl(void* addr, void*) noexcept
		{
			::free(addr);
		}
	};
}
