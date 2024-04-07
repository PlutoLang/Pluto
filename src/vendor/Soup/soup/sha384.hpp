#pragma once

#include "CryptoHashAlgo.hpp"

#include <cstddef>
#include <cstdint>

NAMESPACE_SOUP
{
	struct sha384 : public CryptoHashAlgo<sha384>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30 };
		static constexpr auto DIGEST_BYTES = 48u;
		static constexpr auto BLOCK_BYTES = 128u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL;


		static constexpr unsigned int SEQUENCE_LEN = (1024 / 64);
		static constexpr size_t HASH_LEN = 8;
		static constexpr size_t WORKING_VAR_LEN = 8;
		static constexpr size_t MESSAGE_BLOCK_SIZE = 1024;
		static constexpr size_t CHAR_LEN_BITS = 8;
		static constexpr size_t WORD_LEN = 8;

		static constexpr const uint64_t hPrime[8] = { 0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL, 0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL, 0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL, 0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL };
	};
}
