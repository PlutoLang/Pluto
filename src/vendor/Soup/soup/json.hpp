#pragma once

#include "JsonNode.hpp"
#include "UniquePtr.hpp"

// Convenience includes
#include "JsonArray.hpp"
#include "JsonBool.hpp"
#include "JsonFloat.hpp"
#include "JsonInt.hpp"
#include "JsonNull.hpp"
#include "JsonObject.hpp"
#include "JsonString.hpp"

NAMESPACE_SOUP
{
	struct json
	{
		// Don't do json::decode(...)->... because:
		// 1. UniquePtr manages the lifetime, by using the return value as a temporary like that, you will use-after-free.
		// 2. The returned UniquePtr might be default-constructed in case of a parse error, so you'd be dereferencing a nullptr.
		[[nodiscard]] static UniquePtr<JsonNode> decode(const std::string& data, int max_depth = 100);
		[[nodiscard]] static UniquePtr<JsonNode> decode(const char*& c, int max_depth);

		// specific to soup
		[[nodiscard]] static UniquePtr<JsonNode> binaryDecode(Reader& r);

		// internal
		static void handleLeadingSpace(const char*& c);
		static void handleComment(const char*& c);
	};
}
