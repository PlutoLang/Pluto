#pragma once

#include "JsonNode.hpp"

#include <string>

namespace soup
{
	struct JsonString : public JsonNode
	{
		std::string value{};

		explicit JsonString() noexcept;
		explicit JsonString(std::string&& value) noexcept;
		explicit JsonString(const char*& c);

		[[nodiscard]] std::string encode() const final;

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
