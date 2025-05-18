#pragma once

#include "JsonNode.hpp"

#include <string>

NAMESPACE_SOUP
{
	struct JsonString : public JsonNode
	{
		std::string value{};

		explicit JsonString() noexcept;
		explicit JsonString(const std::string& value) noexcept;
		explicit JsonString(std::string&& value) noexcept;

		bool operator ==(const JsonNode& b) const noexcept final;

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool binaryEncode(Writer& w) const final;

		operator std::string& () noexcept
		{
			return value;
		}

		operator const std::string& () const noexcept
		{
			return value;
		}

		bool operator ==(const std::string& b) const noexcept
		{
			return value == b;
		}

		[[nodiscard]] static size_t getEncodedSize(const char* data, size_t size) noexcept;
		[[nodiscard]] static std::string decodeValue(const char*& c, size_t& s) SOUP_EXCAL;
	};
}
