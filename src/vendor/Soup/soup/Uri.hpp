#pragma once

#include <filesystem>
#include <string>

namespace soup
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

		Uri() = default;
		Uri(const char* url);
		Uri(std::string url);

		[[nodiscard]] bool empty() const noexcept
		{
			return scheme.empty() && host.empty() && port == 0 && user.empty() && pass.empty() && path.empty() && query.empty() && fragment.empty();
		}

		[[nodiscard]] std::string toString() const;

		[[nodiscard]] bool isHttp() const noexcept;
		[[nodiscard]] std::string getRequestPath() const;

		[[nodiscard]] static Uri forFile(std::filesystem::path path) noexcept;
		[[nodiscard]] bool isFile() const noexcept;
		[[nodiscard]] std::string getFilePath() const;

		[[nodiscard]] static std::string data(const char* mime_type, const std::string& contents);
	};
}
