#pragma once

#include "JsonNode.hpp"

namespace soup
{
	struct JsonFloat : public JsonNode
	{
		double value;

		explicit JsonFloat(double value = 0.0) noexcept;

		[[nodiscard]] std::string encode() const final;

		bool binaryEncode(Writer& w) const final;
	};
}
