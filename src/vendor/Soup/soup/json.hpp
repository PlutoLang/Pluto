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
		[[nodiscard]] static UniquePtr<JsonNode> decode(const std::string& data, int max_depth = 100) { return decode(data.data(), data.size(), max_depth); }
		[[nodiscard]] static UniquePtr<JsonNode> decode(const char* data, size_t size, int max_depth = 100);
		static void decode(const char* data, int max_depth) = delete;
		static void decode(const char*& c, int max_depth) = delete;
		static void* decode(const JsonTreeWriter& tw, void* user_data, const char*& c, size_t& s, int max_depth);

		// specific to soup
		[[nodiscard]] static UniquePtr<JsonNode> binaryDecode(Reader& r);

		// internal
		static void handleLeadingSpace(const char*& c, size_t& s);
		static void handleComment(const char*& c, size_t& s);
	};

	struct JsonTreeWriter
	{
		void* (*allocArray)(void* user_data);
		void* (*allocObject)(void* user_data);
		void* (*allocString)(void* user_data, std::string&& value);
		void* (*allocUnescapedString)(void* user_data, const char* data, size_t size) = nullptr;
		void* (*allocInt)(void* user_data, int64_t value);
		void* (*allocFloat)(void* user_data, double value);
		void* (*allocBool)(void* user_data, bool value);
		void* (*allocNull)(void* user_data);
		void(*addToArray)(void* user_data, void* array, void* value);
		void(*addToObject)(void* user_data, void* object, void* key, void* value);
		void(*free)(void* user_data, void* node);

		void(*onArrayFinished)(void* user_data, void* array) = nullptr;
		void(*onObjectFinished)(void* user_data, void* object) = nullptr;
	};
}
