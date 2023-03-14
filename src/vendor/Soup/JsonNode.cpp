#include "JsonNode.hpp"

#include "Exception.hpp"
#include "JsonArray.hpp"
#include "JsonObject.hpp"

namespace soup
{
#if SOUP_CPP20
	std::strong_ordering JsonNode::operator<=>(const JsonNode& b) const
	{
		if (type != b.type)
		{
			return type <=> b.type;
		}
		return encode() <=> b.encode();
	}
#endif

	bool JsonNode::operator==(const JsonNode& b) const
	{
		return type == b.type
			&& encode() == b.encode()
			;
	}

	bool JsonNode::operator!=(const JsonNode& b) const
	{
		return !operator==(b);
	}

	bool JsonNode::operator<(const JsonNode& b) const
	{
		return type < b.type
			|| encode() < b.encode()
			;
	}

	std::string JsonNode::encodePretty(const std::string& prefix) const
	{
		if (isArr())
		{
			return reinterpretAsArr().encodePretty(prefix);
		}
		if (isObj())
		{
			return reinterpretAsObj().encodePretty(prefix);
		}
		return encode();
	}

	bool JsonNode::binaryEncode(Writer& w) const
	{
		return false;
	}

	JsonArray& JsonNode::asArr()
	{
		if (!isArr())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsArr();
	}

	JsonBool& JsonNode::asBool()
	{
		if (!isBool())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsBool();
	}

	JsonFloat& JsonNode::asFloat()
	{
		if (!isFloat())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsFloat();
	}

	JsonInt& JsonNode::asInt()
	{
		if (!isInt())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsInt();
	}

	JsonObject& JsonNode::asObj()
	{
		if (!isObj())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsObj();
	}

	JsonString& JsonNode::asStr()
	{
		if (!isStr())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsStr();
	}

	const JsonArray& JsonNode::asArr() const
	{
		if (!isArr())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsArr();
	}

	const JsonBool& JsonNode::asBool() const
	{
		if (!isBool())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsBool();
	}

	const JsonFloat& JsonNode::asFloat() const
	{
		if (!isFloat())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsFloat();
	}

	const JsonInt& JsonNode::asInt() const
	{
		if (!isInt())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsInt();
	}

	const JsonObject& JsonNode::asObj() const
	{
		if (!isObj())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsObj();
	}

	const JsonString& JsonNode::asStr() const
	{
		if (!isStr())
		{
			throw Exception("JsonNode has unexpected type");
		}
		return reinterpretAsStr();
	}

	bool JsonNode::isArr() const noexcept
	{
		return type == JSON_ARRAY;
	}

	bool JsonNode::isBool() const noexcept
	{
		return type == JSON_BOOL;
	}

	bool JsonNode::isFloat() const noexcept
	{
		return type == JSON_FLOAT;
	}

	bool JsonNode::isInt() const noexcept
	{
		return type == JSON_INT;
	}

	bool JsonNode::isNull() const noexcept
	{
		return type == JSON_NULL;
	}

	bool JsonNode::isObj() const noexcept
	{
		return type == JSON_OBJECT;
	}

	bool JsonNode::isStr() const noexcept
	{
		return type == JSON_STRING;
	}

	JsonArray& JsonNode::reinterpretAsArr() noexcept
	{
		return *reinterpret_cast<JsonArray*>(this);
	}

	JsonBool& JsonNode::reinterpretAsBool() noexcept
	{
		return *reinterpret_cast<JsonBool*>(this);
	}

	JsonFloat& JsonNode::reinterpretAsFloat() noexcept
	{
		return *reinterpret_cast<JsonFloat*>(this);
	}

	JsonInt& JsonNode::reinterpretAsInt() noexcept
	{
		return *reinterpret_cast<JsonInt*>(this);
	}

	JsonObject& JsonNode::reinterpretAsObj() noexcept
	{
		return *reinterpret_cast<JsonObject*>(this);
	}

	JsonString& JsonNode::reinterpretAsStr() noexcept
	{
		return *reinterpret_cast<JsonString*>(this);
	}

	const JsonArray& JsonNode::reinterpretAsArr() const noexcept
	{
		return *reinterpret_cast<const JsonArray*>(this);
	}

	const JsonBool& JsonNode::reinterpretAsBool() const noexcept
	{
		return *reinterpret_cast<const JsonBool*>(this);
	}

	const JsonFloat& JsonNode::reinterpretAsFloat() const noexcept
	{
		return *reinterpret_cast<const JsonFloat*>(this);
	}

	const JsonInt& JsonNode::reinterpretAsInt() const noexcept
	{
		return *reinterpret_cast<const JsonInt*>(this);
	}

	const JsonObject& JsonNode::reinterpretAsObj() const noexcept
	{
		return *reinterpret_cast<const JsonObject*>(this);
	}

	const JsonString& JsonNode::reinterpretAsStr() const noexcept
	{
		return *reinterpret_cast<const JsonString*>(this);
	}
}
