#pragma once

#include "CryptoHashAlgo.hpp"

#include <cstdint>

namespace soup
{
	struct sha512 : public CryptoHashAlgo<sha512>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40 };
		static constexpr auto DIGEST_BYTES = 64u;
		static constexpr auto BLOCK_BYTES = 128u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL;

		static void processBlock(uint64_t block[16], uint64_t h[8]);
	};
}
