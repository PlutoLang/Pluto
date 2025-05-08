#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "base.hpp"
#include "Optional.hpp"

NAMESPACE_SOUP
{
	// This is \r\n line endings land.
	struct MimeMessage
	{
		std::vector<std::string> headers{};
		std::string body{};

		MimeMessage() = default;
		MimeMessage(std::vector<std::string>&& headers, std::string&& body = {});
		MimeMessage(const std::string& data);

		void setBody(std::string body);
		[[nodiscard]] std::string getBody() const;

		void setContentLength();
		void setContentType();

		void loadMessage(const std::string& data);
		[[nodiscard]] bool hasHeader(std::string key) const SOUP_EXCAL { return findHeader(std::move(key)).has_value(); }
		[[nodiscard]] Optional<std::string> findHeader(std::string key) const SOUP_EXCAL;
		void addHeader(std::string line) SOUP_EXCAL;
		void addHeader(std::string key, const std::string& value) SOUP_EXCAL;
		void setHeader(std::string key, const std::string& value) SOUP_EXCAL;
		void removeHeader(std::string key) noexcept;
		static void normaliseHeaderCasingInplace(char* data, size_t size) noexcept;
		void decode() SOUP_EXCAL;

		[[nodiscard]] std::unordered_map<std::string, std::string> getHeaderFields() const SOUP_EXCAL;

		[[nodiscard]] std::string toString() const SOUP_EXCAL;

		[[nodiscard]] std::string getCanonicalisedBody(bool relaxed) const;
	};
}
