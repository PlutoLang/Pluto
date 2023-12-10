#pragma once

#include "JsonNode.hpp"

#include <utility>
#include <vector>

#include "UniquePtr.hpp"

namespace soup
{
	// Note that per JSON spec, object keys must be strings, but for convenience, Soup allows any JSON type as a valid object key type.
	struct JsonObject : public JsonNode
	{
		std::vector<std::pair<UniquePtr<JsonNode>, UniquePtr<JsonNode>>> children{};

		explicit JsonObject() noexcept;
		explicit JsonObject(const char*& c) noexcept;

		[[nodiscard]] std::string encode() const final;
		[[nodiscard]] std::string encodePretty(const std::string& prefix = {}) const;

		bool binaryEncode(Writer& w) const final;

		[[nodiscard]] UniquePtr<JsonNode>* findUp(const JsonNode& k) noexcept;
		[[nodiscard]] JsonNode* find(const JsonNode& k) const noexcept;
		[[nodiscard]] JsonNode* find(std::string k) const noexcept;
		[[nodiscard]] bool contains(const JsonNode& k) const noexcept;
		[[nodiscard]] bool contains(std::string k) const noexcept;
		[[nodiscard]] JsonNode& at(const JsonNode& k) const;
		[[nodiscard]] JsonNode& at(std::string k) const;
		void erase(const JsonNode& k) noexcept;
		void erase(std::string k) noexcept;
		[[nodiscard]] auto begin() noexcept { return children.begin(); }
		[[nodiscard]] auto end() noexcept { return children.end(); }
		[[nodiscard]] auto begin() const noexcept { return children.begin(); }
		[[nodiscard]] auto end() const noexcept { return children.end(); }

		void add(UniquePtr<JsonNode>&& k, UniquePtr<JsonNode>&& v);
		void add(std::string k, std::string v);
		void add(std::string k, const char* v);
		void add(std::string k, int8_t v);
		void add(std::string k, int16_t v);
		void add(std::string k, int32_t v);
		void add(std::string k, int64_t v);
		void add(std::string k, bool v);

		template <typename T, SOUP_RESTRICT(std::is_base_of_v<JsonNode, T>)>
		void add(std::string k, UniquePtr<T>&& v)
		{
			add(soup::make_unique<JsonString>(std::move(k)), std::move(v));
		}
	};
}
