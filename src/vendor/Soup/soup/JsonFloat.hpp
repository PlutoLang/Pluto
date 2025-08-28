#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonFloat : public JsonNode
	{
		double value;

		explicit JsonFloat(double value = 0.0) noexcept
			: JsonNode(), value(value)
		{
		}

		[[nodiscard]] JsonNodeType getType() const noexcept final { return JSON_FLOAT; }

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		bool msgpackEncode(Writer& w) const final;

		operator double() const noexcept
		{
			return value;
		}

		bool operator ==(double b) const noexcept
		{
			return value == b;
		}
	};
}
