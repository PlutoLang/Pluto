#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	struct lzf
	{
		static unsigned int compress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len);
		static unsigned int decompress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len);
	};
}
