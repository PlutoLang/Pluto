#pragma once

#include <string>

#include "base.hpp" // SOUP_EXCAL

namespace soup
{
	struct urlenc
	{
		[[nodiscard]] static std::string encode(const std::string& data) SOUP_EXCAL;
		[[nodiscard]] static std::string encodePath(const std::string& data) SOUP_EXCAL; // Bodge
		[[nodiscard]] static std::string encodePathWithQuery(const std::string& data) SOUP_EXCAL; // Bodge

		[[nodiscard]] static std::string decode(const std::string& data) SOUP_EXCAL;
	};
}
