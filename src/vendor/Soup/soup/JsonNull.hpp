#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonNull : public JsonNode
	{
		explicit JsonNull() noexcept;

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool binaryEncode(Writer& w) const final;
	};
}
