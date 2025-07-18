#pragma once

#include "CryptoHashAlgo.hpp"

#include "fwd.hpp"

NAMESPACE_SOUP
{
	struct sha256 : public CryptoHashAlgo<sha256>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };
		static constexpr auto DIGEST_BYTES = 32u;
		static constexpr auto BLOCK_BYTES = 64u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL { return hash(str.data(), str.size()); }
		[[nodiscard]] static std::string hash(Reader& r) SOUP_EXCAL;

		struct State
		{
			using Hash = sha256;

			uint8_t buffer[BLOCK_BYTES];
			uint32_t state[8];
			uint8_t buffer_counter;
			uint64_t n_bits;

			State() noexcept;

			void append(const void* data, size_t size) noexcept
			{
				for (size_t i = 0; i != size; ++i)
				{
					appendByte(reinterpret_cast<const uint8_t*>(data)[i]);
				}
			}

			void appendByte(uint8_t byte) noexcept
			{
				buffer[buffer_counter++] = byte;
				n_bits += 8;

				if (buffer_counter == BLOCK_BYTES)
				{
					buffer_counter = 0;
					transform();
				}
			}

			void transform() noexcept;
			void finalise() noexcept;

			void getDigest(uint8_t out[DIGEST_BYTES]) const noexcept;
			[[nodiscard]] std::string getDigest() const SOUP_EXCAL;
		};
	};
}
