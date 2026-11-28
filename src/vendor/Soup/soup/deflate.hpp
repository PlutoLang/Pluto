#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

#include "base.hpp"
#include "fwd.hpp"

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
		[[nodiscard]] static DecompressResult gzDecompress(const std::string& data) SOUP_EXCAL { return gzDecompress(data.data(), data.size()); }
		[[nodiscard]] static DecompressResult gzDecompress(const void* data, size_t size) SOUP_EXCAL;

		// versatile API - accepts DEFLATE, gzip & zlib formats

		[[nodiscard]] static DecompressResult decompress(const std::string& compressed_data) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size()); }
		[[nodiscard]] static DecompressResult decompress(const std::string& compressed_data, size_t max_decompressed_size) SOUP_EXCAL { return decompress(compressed_data.data(), compressed_data.size(), max_decompressed_size); }
		[[nodiscard]] static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size) SOUP_EXCAL { return decompress(compressed_data, compressed_data_size, getMaxDecompressedSize(compressed_data, compressed_data_size)); }
		[[nodiscard]] static DecompressResult decompress(const void* compressed_data, size_t compressed_data_size, size_t max_decompressed_size) SOUP_EXCAL;
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

		[[nodiscard]] static std::string decompressZeroTerminated(const std::string& data) SOUP_EXCAL; // For RFC7692-style data where instead of the "final block" bit, an empty block signals the end.

		// encode API

		enum WrapperType
		{
			WT_DEFLATE,
			WT_DEFLATE_STREAM, // BFINAL is always 0 (RFC7692 style)
			WT_ZLIB,
			WT_GZIP,
		};

		// Almost as good as 7z :^)
		[[nodiscard]] static std::string compress(const std::string& data, uint_fast8_t level/*1..9*/ = 6, WrapperType wt = WT_DEFLATE) { return compress(data.data(), data.size(), level, wt); }
		[[nodiscard]] static std::string compress(const void* data, size_t size, uint_fast8_t level/*1..9*/ = 6, WrapperType wt = WT_DEFLATE);

		// Only attempts lz77 + huffman coding without considering cases where uncompressed is better.
		[[nodiscard]] static std::string compressRaw(const std::string& data, uint_fast8_t level/*1..9*/ = 6, WrapperType wt = WT_DEFLATE) { return compressRaw(data.data(), data.size(), level, wt); }
		[[nodiscard]] static std::string compressRaw(const void* data, size_t size, uint_fast8_t level/*1..9*/ = 6, WrapperType wt = WT_DEFLATE);

		// Not very good compared to deflate::compress or even lzf::compress
		[[nodiscard]] static std::string compressHuffman(const std::string& data, WrapperType wt = WT_DEFLATE) { return compressHuffman(reinterpret_cast<const uint8_t*>(data.data()), data.size(), wt); }
		[[nodiscard]] static std::string compressHuffman(const uint8_t* data, size_t size, WrapperType wt = WT_DEFLATE);

		static void writeZlibHeader(StringWriter& sw);
		static void writeGzipHeader(StringWriter& sw);
		static void writeZlibFooter(Writer& w, const void* uncompressed_data, size_t uncompressed_size);
		static void writeGzipFooter(Writer& w, const void* uncompressed_data, size_t uncompressed_size);

		// The input format is specific to Soup; an array of u64_dyn_bp of either a byte value (0..255) or an encoded length-distance pair (see encodeLengthDistancePair).
		static void encodeDynamic(Writer& w, const void* data, size_t size, bool bfinal = true);
		static void encodeStatic(Writer& w, const void* data, size_t size, bool bfinal = true);
		[[nodiscard]] static uint32_t encodeLengthDistancePair(uint_fast16_t length, uint_fast16_t distance) noexcept; // length must be 3..258 and distance must be 1..32768
		static void writeDynamicHuffmanHeader(BitWriter& bw, uint8_t lengths[/*litlen_size+dist_size*/], uint_fast16_t litlen_size /*257..286*/, uint_fast16_t dist_size /*1..32*/, bool bfinal);
		static void writeHuffmanBody(BitWriter& bw, const void* data, size_t size, const uint8_t litlen_lengths[], const uint16_t litlen_codes[], const uint8_t dist_lengths[], const uint16_t dist_codes[]);

		[[nodiscard]] static std::string encodeUncompressed(const void* data, size_t size, WrapperType wt = WT_DEFLATE) SOUP_EXCAL;
		[[nodiscard]] static constexpr size_t encodeUncompressed_size(size_t size, WrapperType wt = WT_DEFLATE) noexcept;
		static void encodeUncompressed(const char* data, size_t size, char out[/* encodeUncompressed_size(size) */], WrapperType wt = WT_DEFLATE) noexcept;
		[[nodiscard]] static constexpr size_t getWrapperOverhead(WrapperType wt) noexcept;
	};

	inline constexpr size_t deflate::encodeUncompressed_size(size_t size, WrapperType wt) noexcept
	{
		const auto blocks = size != 0 ? ((size + 0xfffe) / 0xffff) : 1;
		size = blocks * 5 + size;
		size += getWrapperOverhead(wt);
		return size;
	}

	inline constexpr size_t deflate::getWrapperOverhead(WrapperType wt) noexcept
	{
		if (wt == WT_ZLIB)
		{
			return 6;
		}
		if (wt == WT_GZIP)
		{
			return 18;
		}
		return 0;
	}
}
