#pragma once

#include <cstddef> // size_t

#include "base.hpp"

NAMESPACE_SOUP
{
	[[nodiscard]] void* malloc(size_t size) /* SOUP_EXCAL */;
	[[nodiscard]] void* realloc(void* ptr, size_t new_size) /* SOUP_EXCAL */;
}
