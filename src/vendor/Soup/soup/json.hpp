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

namespace soup
{
	struct json
	{
		// Don't do json::decode(...)->... because:
		// 1. UniquePtr manages the lifetime, by using the return value as a temporary like that, you will use-after-free.
		// 2. The returned UniquePtr might be default-constructed in case of a parse error, so you'd be dereferencing a nullptr.
		[[nodiscard]] static UniquePtr<JsonNode> decode(const std::string& data);
		[[nodiscard]] static UniquePtr<JsonNode> decode(const char*& c);

		[[deprecated]] static void decode(UniquePtr<JsonNode>& out, const std::string& data);
		[[deprecated]] static UniquePtr<JsonNode> decodeForDedicatedVariable(const std::string& data);

		// specific to soup
		static void binaryDecode(UniquePtr<JsonNode>& out, Reader& r);
		[[nodiscard]] static UniquePtr<JsonNode> binaryDecodeForDedicatedVariable(Reader& r);
	};
}
