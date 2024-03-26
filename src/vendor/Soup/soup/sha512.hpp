#pragma once

#include "CryptoHashAlgo.hpp"

#include <cstddef>
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


		static constexpr unsigned int SEQUENCE_LEN = (1024 / 64);
		static constexpr size_t HASH_LEN = 8;
		static constexpr size_t WORKING_VAR_LEN = 8;
		static constexpr size_t MESSAGE_BLOCK_SIZE = 1024;
		static constexpr size_t CHAR_LEN_BITS = 8;
		static constexpr size_t WORD_LEN = 8;

		static constexpr const uint64_t hPrime[8] = { 0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL };
	};
}
