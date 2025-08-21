#include "JsonNode.hpp"

#include "Exception.hpp"
#include "json.hpp"
#include "JsonArray.hpp"
#include "JsonObject.hpp"

NAMESPACE_SOUP
{
#if SOUP_CPP20
	std::strong_ordering JsonNode::operator<=>(const JsonNode& b) const
	{
		if (getType() != b.getType())
		{
			return getType() <=> b.getType();
		}
		return encode() <=> b.encode();
	}
#endif

	bool JsonNode::operator==(const JsonNode& b) const
	{
		return getType() == b.getType()
			&& encode() == b.encode()
			;
	}

	bool JsonNode::operator!=(const JsonNode& b) const
	{
		return !operator==(b);
	}

	bool JsonNode::operator<(const JsonNode& b) const
	{
		return getType() < b.getType()
			|| encode() < b.encode()
			;
	}

	void JsonNode::encodePrettyAndAppendTo(std::string& str, unsigned depth) const SOUP_EXCAL
	{
		if (isArr())
		{
			reinterpretAsArr().encodePrettyAndAppendTo(str, depth);
		}
		else if (isObj())
		{
			reinterpretAsObj().encodePrettyAndAppendTo(str, depth);
		}
		else
		{
			encodeAndAppendTo(str);
		}
	}

	UniquePtr<JsonNode> JsonNode::clone() const SOUP_EXCAL
	{
		return json::decode(encode());
	}

	void JsonNode::throwTypeError()
	{
		SOUP_THROW(Exception("JsonNode has unexpected type"));
	}

	double JsonNode::toFloat() const
	{
		if (isFloat())
		{
			return static_cast<double>(asFloat());
		}
		if (isInt())
		{
			return static_cast<double>(asInt());
		}
		throwTypeError();
	}
}
