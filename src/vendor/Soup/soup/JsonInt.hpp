#pragma once

#include "JsonNode.hpp"

namespace soup
{
	struct JsonInt : public JsonNode
	{
		int64_t value;

		explicit JsonInt(int64_t value = 0) noexcept;

		[[nodiscard]] std::string encode() const final;

		bool binaryEncode(Writer& w) const final;
		
		operator int64_t() const noexcept
		{
			return value;
		}

		bool operator ==(int b) const noexcept
		{
			return value == b;
		}

		bool operator ==(int64_t b) const noexcept
		{
			return value == b;
		}
	};
}
