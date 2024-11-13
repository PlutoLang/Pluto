#include "alloc.hpp"

#include <cstdlib> // malloc, realloc
#include <new> // bad_alloc

NAMESPACE_SOUP
{
	void* malloc(size_t size) /* SOUP_EXCAL */
	{
		void* ptr = ::malloc(size);
		SOUP_IF_LIKELY(ptr)
		{
			return ptr;
		}
		SOUP_THROW(std::bad_alloc{});
	}

	void* realloc(void* ptr, size_t new_size) /* SOUP_EXCAL */
	{
		ptr = ::realloc(ptr, new_size);
		SOUP_IF_LIKELY(ptr)
		{
			return ptr;
		}
		SOUP_THROW(std::bad_alloc{});
	}
}
