#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct deflate
	{
		enum ChecksumState : uint8_t
		{
			CHKSUM_NONE,
			CHKSUM_PASS,
			CHKSUM_FAIL,
		};

		struct DecompressResult
		{
			std::string decompressed{};
			size_t compressed_size = 0;
			ChecksumState checksum_state = CHKSUM_NONE;
		};

		// gzip API - assumes that the entire range data[0]..data[size] is the contents of a .gz file

		struct gzFooter
		{
			uint32_t uncompressed_data_crc32;
			uint32_t uncompressed_size;
		};

		static bool gzReadFooter(const void* data, size_t size, gzFooter& out) noexcept;
		static DecompressResult gzDecompress(const std::string& data) SOUP_EXCAL { return gzDecompress(data.data(), data.size()); }
		static DecompressResult gzDecompress(const void* data, size_t size) SOUP_EXCAL;

		// versatile API - accepts DEFLATE, gzip & zlib formats

		static DecompressResult decompress(const std::string& compressed_data) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size()); }
		static DecompressResult decompress(const std::string& compressed_data, size_t max_decompressed_size) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size(), max_decompressed_size); }
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size) SOUP_EXCAL { return decompress(compressed_data, compressed_data_size, getMaxDecompressedSize(compressed_data, compressed_data_size)); }
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size, size_t max_decompressed_size) SOUP_EXCAL;
		static size_t decompress(const void* compressed_data, size_t compressed_data_size, void* out, /*in/out*/ size_t& decompressed_size, /*out*/ ChecksumState& checksum_state) noexcept; // Returns compressed size or -1 on failure.

		[[nodiscard]] static size_t getMaxDecompressedSize(const void* compressed_data, size_t compressed_data_size) noexcept
		{
			return compressed_data_size * 42;
			// The input that is known to be 42 times bigger uncompressed: https://github.com/Sainan/eve-online-sde/blob/c38c9175cb36b6b058a332c35a0e1f2b6c7b0e7e/controlTowerResources.jsonl.gz
			// However, such a good ratio is very unlikely, so it is advisable to just know the decompressed size upfront if at all possible.
		}

		// raw stream API

		struct Context
		{
			void* _[5];
		};
		static void initContext(Context& ctx, const uint8_t* compressed_data, size_t compressed_data_size) noexcept;
		static unsigned int decompressBlock(Context& ctx, uint8_t out[/*max_decompressed_size*/], size_t current_out_offset, size_t max_decompressed_size, /*out*/ bool& final_block) noexcept; // Returns bytes written to 'out' or -1 on error. When processing multiple related blocks (same sliding window), adjust 'current_out_offset', NOT 'out' or 'max_decompressed_size'!

		static std::string decompressZeroTerminated(const std::string& data) SOUP_EXCAL; // For RFC7692-style data where instead of the "final block" bit, an empty block signals the end.
	};
}
