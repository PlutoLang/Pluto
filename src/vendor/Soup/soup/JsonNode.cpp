#include "JsonNode.hpp"

#include "Exception.hpp"
#include "JsonArray.hpp"
#include "JsonObject.hpp"

NAMESPACE_SOUP
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

	void JsonNode::throwTypeError()
	{
		SOUP_THROW(Exception("JsonNode has unexpected type"));
	}
}
