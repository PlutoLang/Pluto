#pragma once

#include <cstddef>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct deflate
	{
		struct DecompressResult
		{
			std::string decompressed{};
			size_t compressed_size = 0;
			bool checksum_present = false;
			bool checksum_mismatch = false;
		};

		// accepts DEFLATE, gzip & zlib formats
		static DecompressResult decompress(const std::string& compressed_data);
		static DecompressResult decompress(const std::string& compressed_data, size_t max_decompressed_size);
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size);
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size, size_t max_decompressed_size);
	};
}
