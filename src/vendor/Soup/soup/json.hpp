#pragma once

#include <filesystem>

#include "JsonArray.hpp"
#include "JsonBool.hpp"
#include "JsonFloat.hpp"
#include "JsonInt.hpp"
#include "JsonNull.hpp"
#include "JsonObject.hpp"
#include "JsonString.hpp"
#include "UniquePtr.hpp"

NAMESPACE_SOUP
{
	struct json
	{
		// Don't do json::decode(...)->... because:
		// 1. UniquePtr manages the lifetime. By using the return value as a temporary like that, you will use-after-free.
		// 2. The returned UniquePtr might be default-constructed in case of a parse error, so you'd be dereferencing a nullptr.
		[[nodiscard]] static UniquePtr<JsonNode> decode(const std::string& data, int max_depth = 100) { return decode(data.data(), data.size(), max_depth); }
		[[nodiscard]] static UniquePtr<JsonNode> decode(const char* data, size_t size, int max_depth = 100);
		[[nodiscard]] static UniquePtr<JsonNode> decodeFile(const std::filesystem::path& path, int max_depth = 100);
		static void decode(const char* data, int max_depth) = delete;
		static void decode(const char*& c, int max_depth) = delete;
		static void* decode(const JsonTreeWriter& tw, void* user_data, const char*& c, size_t& s, int max_depth);

		// Decodes a MessagePack (https://msgpack.org/) binary stream.
		// If the data represented is not valid JSON (i.e. using extension types), the returned UniquePtr will be default-constructed.
		[[nodiscard]] static UniquePtr<JsonNode> msgpackDecode(Reader& r, int max_depth = 100);
		static void* msgpackDecode(const JsonTreeWriter& tw, void* user_data, Reader& r, int max_depth);

		// internal
		static void handleLeadingSpace(const char*& c, size_t& s);
		static void handleComment(const char*& c, size_t& s);
	};

	struct JsonTreeWriter
	{
		void* (*allocArray)(void* user_data, size_t reserve_size);
		void* (*allocObject)(void* user_data, size_t reserve_size);
		void* (*allocString)(void* user_data, std::string&& value);
		void* (*allocUnescapedString)(void* user_data, const char* data, size_t size);
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

	struct DefaultJsonTreeWriter : public JsonTreeWriter
	{
		DefaultJsonTreeWriter()
		{
			allocArray = [](void*, size_t reserve_size) -> void* { return new JsonArray(reserve_size); };
			allocObject = [](void*, size_t reserve_size) -> void* { return new JsonObject(reserve_size); };
			allocString = [](void*, std::string&& value) -> void* { return new JsonString(std::move(value)); };
			allocUnescapedString = [](void*, const char* data, size_t size) -> void* { return new JsonString(data, size); };
			allocInt = [](void*, int64_t value) -> void* { return new JsonInt(value); };
			allocFloat = [](void*, double value) -> void* { return new JsonFloat(value); };
			allocBool = [](void*, bool value) -> void* { return new JsonBool(value); };
			allocNull = [](void*) -> void* { return new JsonNull(); };
			addToArray = [](void*, void* arr, void* value) -> void { ((JsonArray*)arr)->children.emplace_back((JsonNode*)value); };
			addToObject = [](void*, void* obj, void* key, void* value) -> void { ((JsonObject*)obj)->children.emplace_back((JsonNode*)key, (JsonNode*)value); };
			free = [](void*, void* node) -> void { delete (JsonNode*)node; };
		}
	};
}
