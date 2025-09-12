#pragma once

#include "base.hpp"
#include "fwd.hpp"

#include "JsonNodeType.hpp"
#include <string>

NAMESPACE_SOUP
{
	struct JsonNode
	{
		[[nodiscard]] virtual JsonNodeType getType() const noexcept = 0;
		virtual ~JsonNode() = default;

#if SOUP_CPP20
		std::strong_ordering operator <=>(const JsonNode& b) const;
#endif
		virtual bool operator ==(const JsonNode& b) const;
		bool operator !=(const JsonNode& b) const;
		bool operator <(const JsonNode& b) const;

		[[nodiscard]] std::string encode() const SOUP_EXCAL;
		[[nodiscard]] std::string encodePretty() const SOUP_EXCAL;
		virtual void encodeAndAppendTo(std::string& str) const SOUP_EXCAL = 0;
		void encodePrettyAndAppendTo(std::string& str, unsigned depth = 0) const SOUP_EXCAL;

		// Encodes a MessagePack (https://msgpack.org/) binary stream.
		virtual bool msgpackEncode(Writer& w) const = 0;

		[[nodiscard]] UniquePtr<JsonNode> clone() const SOUP_EXCAL;

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

		[[nodiscard]] double toFloat() const; // valid for int & float

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

	protected:
		[[noreturn]] static void throwTypeError();
	};

	inline std::string JsonNode::encode() const SOUP_EXCAL
	{
		std::string str;
		encodeAndAppendTo(str);
		return str;
	}

	inline std::string JsonNode::encodePretty() const SOUP_EXCAL
	{
		std::string str;
		encodePrettyAndAppendTo(str);
		return str;
	}

	inline JsonArray& JsonNode::asArr()
	{
		SOUP_IF_UNLIKELY (!isArr())
		{
			throwTypeError();
		}
		return reinterpretAsArr();
	}

	inline JsonBool& JsonNode::asBool()
	{
		SOUP_IF_UNLIKELY (!isBool())
		{
			throwTypeError();
		}
		return reinterpretAsBool();
	}

	inline JsonFloat& JsonNode::asFloat()
	{
		SOUP_IF_UNLIKELY (!isFloat())
		{
			throwTypeError();
		}
		return reinterpretAsFloat();
	}

	inline JsonInt& JsonNode::asInt()
	{
		SOUP_IF_UNLIKELY (!isInt())
		{
			throwTypeError();
		}
		return reinterpretAsInt();
	}

	inline JsonObject& JsonNode::asObj()
	{
		SOUP_IF_UNLIKELY (!isObj())
		{
			throwTypeError();
		}
		return reinterpretAsObj();
	}

	inline JsonString& JsonNode::asStr()
	{
		SOUP_IF_UNLIKELY (!isStr())
		{
			throwTypeError();
		}
		return reinterpretAsStr();
	}

	inline const JsonArray& JsonNode::asArr() const
	{
		SOUP_IF_UNLIKELY (!isArr())
		{
			throwTypeError();
		}
		return reinterpretAsArr();
	}

	inline const JsonBool& JsonNode::asBool() const
	{
		SOUP_IF_UNLIKELY (!isBool())
		{
			throwTypeError();
		}
		return reinterpretAsBool();
	}

	inline const JsonFloat& JsonNode::asFloat() const
	{
		SOUP_IF_UNLIKELY (!isFloat())
		{
			throwTypeError();
		}
		return reinterpretAsFloat();
	}

	inline const JsonInt& JsonNode::asInt() const
	{
		SOUP_IF_UNLIKELY (!isInt())
		{
			throwTypeError();
		}
		return reinterpretAsInt();
	}

	inline const JsonObject& JsonNode::asObj() const
	{
		SOUP_IF_UNLIKELY (!isObj())
		{
			throwTypeError();
		}
		return reinterpretAsObj();
	}

	inline const JsonString& JsonNode::asStr() const
	{
		SOUP_IF_UNLIKELY (!isStr())
		{
			throwTypeError();
		}
		return reinterpretAsStr();
	}

	inline bool JsonNode::isArr() const noexcept
	{
		return getType() == JSON_ARRAY;
	}

	inline bool JsonNode::isBool() const noexcept
	{
		return getType() == JSON_BOOL;
	}

	inline bool JsonNode::isFloat() const noexcept
	{
		return getType() == JSON_FLOAT;
	}

	inline bool JsonNode::isInt() const noexcept
	{
		return getType() == JSON_INT;
	}

	inline bool JsonNode::isNull() const noexcept
	{
		return getType() == JSON_NULL;
	}

	inline bool JsonNode::isObj() const noexcept
	{
		return getType() == JSON_OBJECT;
	}

	inline bool JsonNode::isStr() const noexcept
	{
		return getType() == JSON_STRING;
	}

	// Using reinterpret_cast instead of static_cast because not all of these types may be known

	inline JsonArray& JsonNode::reinterpretAsArr() noexcept
	{
		return *reinterpret_cast<JsonArray*>(this);
	}

	inline JsonBool& JsonNode::reinterpretAsBool() noexcept
	{
		return *reinterpret_cast<JsonBool*>(this);
	}

	inline JsonFloat& JsonNode::reinterpretAsFloat() noexcept
	{
		return *reinterpret_cast<JsonFloat*>(this);
	}

	inline JsonInt& JsonNode::reinterpretAsInt() noexcept
	{
		return *reinterpret_cast<JsonInt*>(this);
	}

	inline JsonObject& JsonNode::reinterpretAsObj() noexcept
	{
		return *reinterpret_cast<JsonObject*>(this);
	}

	inline JsonString& JsonNode::reinterpretAsStr() noexcept
	{
		return *reinterpret_cast<JsonString*>(this);
	}

	inline const JsonArray& JsonNode::reinterpretAsArr() const noexcept
	{
		return *reinterpret_cast<const JsonArray*>(this);
	}

	inline const JsonBool& JsonNode::reinterpretAsBool() const noexcept
	{
		return *reinterpret_cast<const JsonBool*>(this);
	}

	inline const JsonFloat& JsonNode::reinterpretAsFloat() const noexcept
	{
		return *reinterpret_cast<const JsonFloat*>(this);
	}

	inline const JsonInt& JsonNode::reinterpretAsInt() const noexcept
	{
		return *reinterpret_cast<const JsonInt*>(this);
	}

	inline const JsonObject& JsonNode::reinterpretAsObj() const noexcept
	{
		return *reinterpret_cast<const JsonObject*>(this);
	}

	inline const JsonString& JsonNode::reinterpretAsStr() const noexcept
	{
		return *reinterpret_cast<const JsonString*>(this);
	}
}
