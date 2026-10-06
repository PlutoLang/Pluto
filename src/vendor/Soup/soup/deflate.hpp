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

		// accepts DEFLATE, gzip & zlib formats
		static DecompressResult decompress(const std::string& compressed_data) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size()); }
		static DecompressResult decompress(const std::string& compressed_data, size_t max_decompressed_size) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size(), max_decompressed_size); }
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size) SOUP_EXCAL { return decompress(compressed_data, compressed_data_size, getMaxDecompressedSize(compressed_data, compressed_data_size)); }
		static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size, size_t max_decompressed_size) SOUP_EXCAL;
		static size_t decompress(const void* compressed_data, size_t compressed_data_size, void* out, /*in/out*/ size_t& decompressed_size, /*out*/ ChecksumState& checksum_state) noexcept; // Returns compressed size or -1 on failure.

		[[nodiscard]] static size_t getMaxDecompressedSize(const void* compressed_data, size_t compressed_data_size) noexcept
		{
			return compressed_data_size * SOUP_DEFLATE_MAX_COMPRESSED_RATIO;
		}

		struct Context
		{
			void* _[5];
		};
		static void initContext(Context& ctx, const uint8_t* compressed_data, size_t compressed_data_size) noexcept;
		static unsigned int decompressBlock(Context& ctx, uint8_t out[/*max_decompressed_size*/], size_t current_out_offset, size_t max_decompressed_size, /*out*/ bool& final_block) noexcept; // Returns bytes written to 'out' or -1 on error. When processing multiple related blocks (same sliding window), adjust 'current_out_offset', NOT 'out' or 'max_decompressed_size'!

		static std::string decompressZeroTerminated(const std::string& data) SOUP_EXCAL; // For RFC7692-style data where instead of the "final block" bit, an empty block signals the end.
	};
}
