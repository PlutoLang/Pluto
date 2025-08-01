#pragma once

#include <filesystem>
#include <string>

#include "base.hpp" // SOUP_EXCAL

NAMESPACE_SOUP
{
	struct Uri
	{
		std::string scheme{};
		std::string host{};
		uint16_t port{};
		std::string user{};
		std::string pass{};
		std::string path{};
		std::string query{};
		std::string fragment{};

		Uri() noexcept = default;
		Uri(const char* url) SOUP_EXCAL;
		Uri(std::string url) SOUP_EXCAL;

		[[nodiscard]] bool empty() const noexcept
		{
			return scheme.empty() && host.empty() && port == 0 && user.empty() && pass.empty() && path.empty() && query.empty() && fragment.empty();
		}

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

		[[nodiscard]] bool isHttp() const noexcept;
		[[nodiscard]] std::string getHost() const noexcept; // host[:port]
		[[nodiscard]] uint16_t getPort() const noexcept;
		[[nodiscard]] std::string getRequestPath() const SOUP_EXCAL;

		[[nodiscard]] static Uri forFile(std::filesystem::path path) SOUP_EXCAL;
		[[nodiscard]] bool isFile() const noexcept;
		[[nodiscard]] std::string getFilePath() const SOUP_EXCAL;

		[[nodiscard]] static std::string data(const char* mime_type, const std::string& contents) SOUP_EXCAL;
	};
}
