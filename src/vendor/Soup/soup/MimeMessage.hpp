#pragma once

#include <string>
#include <unordered_map>

#include "base.hpp"

NAMESPACE_SOUP
{
	// This is \r\n line endings land.
	struct MimeMessage
	{
		std::unordered_map<std::string, std::string> header_fields{};
		std::string body{};

		MimeMessage() = default;
		MimeMessage(std::unordered_map<std::string, std::string>&& header_fields, std::string&& body = {});
		MimeMessage(const std::string& data);

		void setBody(std::string body);
		[[nodiscard]] std::string getBody() const;

		void setContentLength();
		void setContentType();

		void loadMessage(const std::string& data);
		[[nodiscard]] bool hasHeader(const std::string& key) const noexcept;
		[[nodiscard]] std::string* findHeader(std::string key) noexcept;
		[[nodiscard]] const std::string* findHeader(std::string key) const noexcept;
		void addHeader(const std::string& line) SOUP_EXCAL;
		void setHeader(const std::string& key, const std::string& value) SOUP_EXCAL;
		[[nodiscard]] static std::string normaliseHeaderCasing(const std::string& key) SOUP_EXCAL;
		void decode();

		[[nodiscard]] std::string toString() const;

		[[nodiscard]] std::string getCanonicalisedBody(bool relaxed) const;
	};
}
