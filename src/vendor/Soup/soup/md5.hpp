#pragma once

#include "CryptoHashAlgo.hpp"

#include <cstdint>
#include <string>

NAMESPACE_SOUP
{
	struct md5 : public CryptoHashAlgo<md5>
	{
		static constexpr auto DIGEST_BYTES = 16u;
		static constexpr auto BLOCK_BYTES = 64u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL { return hash(str.data(), str.size()); }

		struct State
		{
			uint8_t buffer[BLOCK_BYTES];
			uint32_t state[4];
			uint64_t n_bytes;

			State();

			void append(const void* data, size_t size) noexcept;

			void appendByte(uint8_t byte) noexcept
			{
				return append(&byte, 1);
			}

			void transform() noexcept;
			void finalise() noexcept;

			void getDigest(uint8_t out[DIGEST_BYTES]) const noexcept;
			[[nodiscard]] std::string getDigest() const SOUP_EXCAL;
		};
	};
}
