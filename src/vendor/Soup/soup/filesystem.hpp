#pragma once

#include <filesystem>
#include <string>

#include "base.hpp"

NAMESPACE_SOUP
{
	struct filesystem
	{
		[[nodiscard]] static std::filesystem::path u8path(const std::string& str);

		[[nodiscard]] static bool exists_case_sensitive(const std::filesystem::path& p);
		[[nodiscard]] static intptr_t filesize(const std::filesystem::path& path); // returns -1 on error

		[[nodiscard]] static std::filesystem::path tempfile(const std::string& ext = {});
		[[nodiscard]] static std::filesystem::path getProgramData() SOUP_EXCAL;

		[[nodiscard]] static const void* createFileMapping(const std::filesystem::path& path, size_t& out_len) noexcept;
		static void destroyFileMapping(const void* addr, size_t len) noexcept;
	};
}
