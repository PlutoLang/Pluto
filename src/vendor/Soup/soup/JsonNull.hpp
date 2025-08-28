#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonNull : public JsonNode
	{
		explicit JsonNull() noexcept
			: JsonNode()
		{
		}

		[[nodiscard]] JsonNodeType getType() const noexcept final { return JSON_NULL; }

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool msgpackEncode(Writer& w) const final;
	};
}
