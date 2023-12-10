#pragma once

#include "TreeNode.hpp"

#include "base.hpp"
#include "fwd.hpp"

#include "JsonNodeType.hpp"
#include <string>

namespace soup
{
	struct JsonNode : public TreeNode
	{
		JsonNodeType type;

		explicit JsonNode(JsonNodeType type) noexcept
			: type(type)
		{
		}

#if SOUP_CPP20
		std::strong_ordering operator <=>(const JsonNode& b) const;
#endif
		bool operator ==(const JsonNode& b) const;
		bool operator !=(const JsonNode& b) const;
		bool operator <(const JsonNode& b) const;

		[[nodiscard]] virtual std::string encode() const = 0;
		[[nodiscard]] std::string encodePretty(const std::string& prefix = {}) const;

		virtual bool binaryEncode(Writer& w) const; // specific to soup

		// Type casts; will throw if node is of different type.
		[[nodiscard]] JsonArray& asArr();
		[[nodiscard]] JsonBool& asBool();
		[[nodiscard]] JsonFloat& asFloat();
		[[nodiscard]] JsonInt& asInt();
		[[nodiscard]] JsonObject& asObj();
		[[nodiscard]] JsonString& asStr();
		[[nodiscard]] const JsonArray& asArr() const;
		[[nodiscard]] const JsonBool& asBool() const;
		[[nodiscard]] const JsonFloat& asFloat() const;
		[[nodiscard]] const JsonInt& asInt() const;
		[[nodiscard]] const JsonObject& asObj() const;
		[[nodiscard]] const JsonString& asStr() const;

		// Type checks.
		[[nodiscard]] bool isArr() const noexcept;
		[[nodiscard]] bool isBool() const noexcept;
		[[nodiscard]] bool isFloat() const noexcept;
		[[nodiscard]] bool isInt() const noexcept;
		[[nodiscard]] bool isNull() const noexcept;
		[[nodiscard]] bool isObj() const noexcept;
		[[nodiscard]] bool isStr() const noexcept;

		// Reinterpret casts; no error considerations.
		[[nodiscard]] JsonArray& reinterpretAsArr() noexcept;
		[[nodiscard]] JsonBool& reinterpretAsBool() noexcept;
		[[nodiscard]] JsonFloat& reinterpretAsFloat() noexcept;
		[[nodiscard]] JsonInt& reinterpretAsInt() noexcept;
		[[nodiscard]] JsonObject& reinterpretAsObj() noexcept;
		[[nodiscard]] JsonString& reinterpretAsStr() noexcept;
		[[nodiscard]] const JsonArray& reinterpretAsArr() const noexcept;
		[[nodiscard]] const JsonBool& reinterpretAsBool() const noexcept;
		[[nodiscard]] const JsonFloat& reinterpretAsFloat() const noexcept;
		[[nodiscard]] const JsonInt& reinterpretAsInt() const noexcept;
		[[nodiscard]] const JsonObject& reinterpretAsObj() const noexcept;
		[[nodiscard]] const JsonString& reinterpretAsStr() const noexcept;
	};
}
