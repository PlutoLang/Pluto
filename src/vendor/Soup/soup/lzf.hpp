#pragma once

#include "base.hpp"

NAMESPACE_SOUP
{
	struct lzf
	{
		static unsigned int compress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len);
		static unsigned int decompress(const void* const in_data, unsigned int in_len, void* out_data, unsigned int out_len);

		[[nodiscard]] static unsigned int getMaxCompressedSize(const void* data, unsigned int size)
		{
			return size + (size >> 5) + 2;
		}

		[[nodiscard]] static unsigned int getMaxDecompressedSize(const void* data, unsigned int size)
		{
			return size << 1;
		}
	};
}
