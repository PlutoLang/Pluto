#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct adler32
	{
		static constexpr uint32_t INITIAL = 1;

		[[nodiscard]] static uint32_t hash(const std::string& data);
		[[nodiscard]] static uint32_t hash(const char* data, size_t size);
		[[nodiscard]] static uint32_t hash(const uint8_t* data, size_t size, uint32_t init = INITIAL);
	};
}
