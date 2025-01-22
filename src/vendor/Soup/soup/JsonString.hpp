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
		explicit JsonString(const char*& c) SOUP_EXCAL;

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
	};
}
