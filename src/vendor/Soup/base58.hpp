#pragma once

#include <string>

namespace soup
{
	// Adapted from https://github.com/bitcoin/bitcoin/blob/master/src/base58.cpp

	struct base58
	{
		[[nodiscard]] static std::string decode(const std::string& in);
	};
}
