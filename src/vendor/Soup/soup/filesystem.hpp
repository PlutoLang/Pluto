#pragma once

#include <filesystem>
#include <string>

namespace soup
{
	struct filesystem
	{
		[[nodiscard]] static std::filesystem::path u8path(const std::string& str);
		[[nodiscard]] static bool exists_case_sensitive(const std::filesystem::path& p);
	};
}
