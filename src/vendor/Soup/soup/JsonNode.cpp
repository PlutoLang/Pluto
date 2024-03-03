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

	std::string JsonNode::encode() const SOUP_EXCAL
	{
		std::string str;
		encodeAndAppendTo(str);
		return str;
	}

	std::string JsonNode::encodePretty(const std::string& prefix) const SOUP_EXCAL
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

	void JsonNode::throwTypeError()
	{
		SOUP_THROW(Exception("JsonNode has unexpected type"));
	}
}
