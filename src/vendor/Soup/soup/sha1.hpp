#pragma once

#include "base.hpp"
#include "fwd.hpp"
#include "CryptoHashAlgo.hpp"

NAMESPACE_SOUP
{
	struct sha1 : public CryptoHashAlgo<sha1>
	{
		static constexpr unsigned char OID[] = { 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14 };
		static constexpr auto DIGEST_BYTES = 20u;
		static constexpr auto BLOCK_BYTES = 64u;

		[[nodiscard]] static std::string hash(const std::string& str) SOUP_EXCAL;
		[[nodiscard]] static std::string hash(Reader& r) SOUP_EXCAL;
	};
}
