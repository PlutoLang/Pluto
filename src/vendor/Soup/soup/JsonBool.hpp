#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonBool : public JsonNode
	{
		bool value;

		explicit JsonBool(bool value = false) noexcept
			: JsonNode(), value(value)
		{
		}

		[[nodiscard]] JsonNodeType getType() const noexcept final { return JSON_BOOL; }

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool msgpackEncode(Writer& w) const final;

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
