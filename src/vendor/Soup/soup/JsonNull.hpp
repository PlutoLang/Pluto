#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonNull : public JsonNode
	{
		explicit JsonNull() noexcept;

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool msgpackEncode(Writer& w) const final;
	};
}
