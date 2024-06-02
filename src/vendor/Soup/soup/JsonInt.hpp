#pragma once

#include "JsonNode.hpp"

NAMESPACE_SOUP
{
	struct JsonInt : public JsonNode
	{
		int64_t value;

		explicit JsonInt(int64_t value = 0) noexcept;

		bool operator ==(const JsonNode& b) const noexcept final;

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
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
