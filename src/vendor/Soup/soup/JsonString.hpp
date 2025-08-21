#pragma once

#include "JsonNode.hpp"

#include <string>

NAMESPACE_SOUP
{
	struct JsonString : public JsonNode
	{
		std::string value{};

		explicit JsonString() noexcept
			: JsonNode()
		{
		}

		explicit JsonString(const std::string& value) noexcept
			: JsonNode(), value(value)
		{
		}

		explicit JsonString(std::string&& value) noexcept
			: JsonNode(), value(std::move(value))
		{
		}

		explicit JsonString(const char* data, size_t size) noexcept
			: JsonNode(), value(data, size)
		{
		}

		[[nodiscard]] JsonNodeType getType() const noexcept final { return JSON_STRING; }

		bool operator ==(const JsonNode& b) const noexcept final;

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		static void encodeValue(std::string& str, const std::string& value) SOUP_EXCAL { return encodeValue(str, value.data(), value.size()); }
		static void encodeValue(std::string& str, const char* data, size_t size) SOUP_EXCAL;
		bool msgpackEncode(Writer& w) const final;

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
		static void decodeValue(std::string& value, const char*& c, size_t& s) SOUP_EXCAL;
	};
}
