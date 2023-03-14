#pragma once

#include "JsonNode.hpp"

namespace soup
{
	struct JsonBool : public JsonNode
	{
		bool value;

		explicit JsonBool() noexcept;
		explicit JsonBool(bool value) noexcept;

		[[nodiscard]] std::string encode() const final;

		bool binaryEncode(Writer& w) const final;

		operator bool() const noexcept
		{
			return value;
		}

		bool operator == (bool b) const noexcept
		{
			return value == b;
		}
	};
}
