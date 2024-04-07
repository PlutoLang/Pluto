#pragma once

#include "fwd.hpp"

#include <string>

NAMESPACE_SOUP
{
	// Made with code from:
	// - https://github.com/mjg59/tpmtotp
	// - https://github.com/sipa/bech32

	struct base32
	{
		[[nodiscard]] static constexpr size_t getEncodedLength(size_t len)
		{
			return (len / 5) * 8 + (len % 5 ? 8 : 0);
		}

		[[nodiscard]] static constexpr size_t getDecodedLength(size_t len)
		{
			return (len / 8) * 5;
		}

		[[nodiscard]] static std::string encode(const std::string& in, bool pad = true);
		[[nodiscard]] static std::string encode(const std::string& in, bool pad, const char* alpha);
		[[nodiscard]] static std::string decode(const std::string& in);
	};
}
