#pragma once

#include "JsonNode.hpp"

#include "UniquePtr.hpp"
#include <vector>

NAMESPACE_SOUP
{
	struct JsonArrayIterator
	{
		const JsonArray* arr;
		size_t i;

		[[nodiscard]] JsonNode& operator*() const noexcept;

		[[nodiscard]] bool operator==(const JsonArrayIterator& b) const noexcept
		{
			return i == b.i;
		}

		[[nodiscard]] bool operator!=(const JsonArrayIterator& b) const noexcept
		{
			return i != b.i;
		}

		void operator++() noexcept
		{
			++i;
		}
	};

	struct JsonArray : public JsonNode
	{
		std::vector<UniquePtr<JsonNode>> children{};

		explicit JsonArray() noexcept;
		explicit JsonArray(const char*& c, int max_depth);

		void encodeAndAppendTo(std::string& str) const SOUP_EXCAL final;
		void encodePrettyAndAppendTo(std::string& str, unsigned depth = 0) const SOUP_EXCAL;

		bool binaryEncode(Writer& w) const final;

		[[nodiscard]] JsonNode& at(size_t i) const;
		void clear() noexcept;
		[[nodiscard]] JsonArrayIterator begin() const noexcept;
		[[nodiscard]] JsonArrayIterator end() const noexcept;

		[[nodiscard]] size_t size() const noexcept
		{
			return children.size();
		}
	};
}
