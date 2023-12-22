#pragma once

#include <string>
#include <unordered_map>

namespace soup
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

		void setContentLength();
		void setContentType();

		void loadMessage(const std::string& data);
		[[nodiscard]] bool hasHeader(const std::string& key);
		[[nodiscard]] std::string* findHeader(std::string key);
		void addHeader(const std::string& line);
		void setHeader(const std::string& key, const std::string& value);
		[[nodiscard]] static std::string normaliseHeaderCasing(const std::string& key);
		void decode();

		[[nodiscard]] std::string toString() const;

		[[nodiscard]] std::string getCanonicalisedBody(bool relaxed) const;
	};
}
