#pragma once

#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	[[nodiscard]] std::string ripemd160(const void* data, size_t size);
	[[nodiscard]] std::string ripemd160(const std::string& in);
}
