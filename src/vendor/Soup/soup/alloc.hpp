#pragma once

#include "base.hpp"

#include <cstdlib> // free

NAMESPACE_SOUP
{
	[[nodiscard]] void* malloc(size_t size) /* SOUP_EXCAL */;
	[[nodiscard]] void* realloc(void* ptr, size_t new_size) /* SOUP_EXCAL */;
	inline void free(void* ptr) noexcept { ::free(ptr); }
}
