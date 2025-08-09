#pragma once

#include "CryptoHashAlgo.hpp"
#include "sha512.hpp"

NAMESPACE_SOUP
{
	struct sha384 : public CryptoHashAlgo<sha384>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30 };
		static constexpr auto DIGEST_BYTES = 48u;
		static constexpr auto BLOCK_BYTES = 128u;

		[[nodiscard]] static std::string hash(const void* data, size_t len) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL { return hash(str.data(), str.size()); }

		struct State : public sha512::State
		{
			using Hash = sha384;

			State() noexcept;

			void getDigest(uint8_t out[DIGEST_BYTES]) const noexcept;
			[[nodiscard]] std::string getDigest() const SOUP_EXCAL;
		};
	};
}
