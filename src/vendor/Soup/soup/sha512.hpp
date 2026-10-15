#pragma once

#include "CryptoHashAlgo.hpp"

#include <cstddef>
#include <cstdint>

NAMESPACE_SOUP
{
	struct sha512 : public CryptoHashAlgo<sha512>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40 };
		static constexpr auto DIGEST_BYTES = 64u;
		static constexpr auto BLOCK_BYTES = 128u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL { return hash(str.data(), str.size()); }

		struct State
		{
			using Hash = sha512;

			uint8_t buffer[BLOCK_BYTES];
			uint64_t state[8];
			uint64_t n_bytes;

			State() noexcept;

			void append(const void* data, size_t size) noexcept
			{
				if (!size)
				{
					return;
				}

				auto left = n_bytes % BLOCK_BYTES;
				auto fill = BLOCK_BYTES - left;

				n_bytes += size;

				if (left && size >= fill)
				{
					memcpy(buffer + left, data, fill);
					transform();
					data = (uint8_t*)data + fill;
					size -= fill;
					left = 0;
				}

				while (size >= BLOCK_BYTES)
				{
					memcpy(buffer, data, BLOCK_BYTES);
					transform();
					data = (uint8_t*)data + BLOCK_BYTES;
					size -= BLOCK_BYTES;
				}

				if (size)
				{
					memcpy(buffer + left, data, size);
				}
			}

			void appendByte(uint8_t byte) noexcept
			{
				append(&byte, 1);
			}

			void transform() noexcept;
			void finalise() noexcept;

			void getDigest(uint8_t out[DIGEST_BYTES]) const noexcept;
			[[nodiscard]] std::string getDigest() const SOUP_EXCAL;
		};
	};
}
