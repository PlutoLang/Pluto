#pragma once

#include <string>

namespace soup
{
	struct urlenc
	{
		[[nodiscard]] static std::string encode(const std::string& data);
		[[nodiscard]] static std::string encodePath(const std::string& data);
		[[nodiscard]] static std::string encodePathWithQuery(const std::string& data);

		[[nodiscard]] static std::string decode(const std::string& data);
	};
}
