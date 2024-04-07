#pragma once

#include <string>
#include <vector>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct pem
	{
		[[nodiscard]] static std::string encode(const std::string& label, const std::string& bin);
		[[nodiscard]] static std::string decode(std::string in);
		[[nodiscard]] static std::string decodeUnpacked(std::string in);

		[[nodiscard]] static std::vector<std::string> decodeChain(const std::string& str);
	};
}
