#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonNull : public JsonNode
	{
		explicit JsonNull() noexcept
			: JsonNode(JSON_NULL)
		{
		}

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool msgpackEncode(Writer& w) const final;
	};
}
