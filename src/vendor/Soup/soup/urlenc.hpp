#pragma once

#include <string>

#include "base.hpp" // SOUP_EXCAL

NAMESPACE_SOUP
{
	struct urlenc
	{
		[[nodiscard]] static std::string encode(const std::string& data) SOUP_EXCAL;
		[[nodiscard]] static std::string encodePath(const std::string& data) SOUP_EXCAL; // Bodge
		[[nodiscard]] static std::string encodePathWithQuery(const std::string& data) SOUP_EXCAL; // Bodge

		[[nodiscard]] static std::string decode(const std::string& data) SOUP_EXCAL { return decode(data.begin(), data.end()); }
		[[nodiscard]] static std::string decode(const std::string::const_iterator begin, const std::string::const_iterator end) SOUP_EXCAL;
	};
}
