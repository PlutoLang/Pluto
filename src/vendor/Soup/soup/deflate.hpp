#pragma once

#include <cstddef>
#include <string>

#include "base.hpp"
#include "tunables.hpp"

NAMESPACE_SOUP
{
	inline SOUP_TUNABLE(uint32_t, SOUP_DEFLATE_MAX_COMPRESSED_RATIO) = 30;

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

		[[nodiscard]] static size_t getMaxDecompressedSize(const void* compressed_data, size_t compressed_data_size)
		{
			return compressed_data_size * SOUP_DEFLATE_MAX_COMPRESSED_RATIO;
		}

		struct Context
		{
			void* _[5];
		};
		static void initContext(Context& ctx, const uint8_t* compressed_data, size_t compressed_data_size);
		static unsigned int decompressBlock(Context& ctx, uint8_t out[/*max_decompressed_size*/], size_t current_out_offset, size_t max_decompressed_size, /*out*/ bool& final_block); // Returns bytes written to 'out' or -1 on error. When processing multiple related blocks (same sliding window), adjust 'current_out_offset', NOT 'out' or 'max_decompressed_size'!

		static std::string decompressZeroTerminated(const std::string& data); // For RFC7692 style data where instead of the "final block" bit, an empty block signals the end.
	};
}
