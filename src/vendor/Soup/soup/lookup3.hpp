#pragma once

#include "base.hpp"

#include <cstdint>
#include <string>

NAMESPACE_SOUP
{
	struct lookup3
	{
		static void hashlittle2(const void* data, size_t size, uint32_t& pc /* IN: primary initval, OUT: primary hash */, uint32_t& pb /* IN: secondary initval, OUT: secondary hash */);

		[[nodiscard]] static uint64_t hash64(const void* data, size_t size)
		{
			uint32_t pc = 0;
			uint32_t pb = 0;
			hashlittle2(data, size, pc, pb);
			return ((uint64_t)pb << 32) | pc;
		}

		[[nodiscard]] static uint64_t hash64(const std::string& str)
		{
			return hash64(str.data(), str.size());
		}
	};
}
