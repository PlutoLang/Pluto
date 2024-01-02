#pragma once

#include "fwd.hpp"

#include <cstdint>
#include <string>

namespace soup
{
	struct crc32
	{
		static constexpr uint32_t INITIAL = 0;

		[[nodiscard]] static uint32_t hash(Reader& r);
		[[nodiscard]] static uint32_t hash(const std::string& data);
		[[nodiscard]] static uint32_t hash(const uint8_t* data, size_t size, uint32_t init = INITIAL);
	};
}
