#include "JsonArray.hpp"

#include "json.hpp"
#include "string.hpp"
#include "Writer.hpp"

NAMESPACE_SOUP
{
	JsonNode& JsonArrayIterator::operator*() const noexcept
	{
		return arr->at(i);
	}

	void JsonArray::encodeAndAppendTo(std::string& str) const SOUP_EXCAL
	{
		str.push_back('[');
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			(*i)->encodeAndAppendTo(str);
			if (i != children.end() - 1)
			{
				str.push_back(',');
			}
		}
		str.push_back(']');
	}

	void JsonArray::encodePrettyAndAppendTo(std::string& str, unsigned depth) const SOUP_EXCAL
	{
		if (children.empty())
		{
			str.append("[]");
		}
		else
		{
			const auto child_depth = (depth + 1);
			str.append("[\n");
			for (auto i = children.begin(); i != children.end(); ++i)
			{
				str.append(child_depth * 4, ' ');
				(*i)->encodePrettyAndAppendTo(str, child_depth);
				if (i != children.end() - 1)
				{
					str.push_back(',');
				}
				str.push_back('\n');
			}
			str.append(depth * 4, ' ');
			str.push_back(']');
		}
	}

	bool JsonArray::msgpackEncode(Writer& w) const
	{
		if (children.size() <= 0b1111)
		{
			uint8_t b = 0b1001'0000 | (uint8_t)children.size();
			SOUP_RETHROW_FALSE(w.u8(b));
		}
		else if (children.size() <= 0xffff)
		{
			uint8_t b = 0xdc;
			SOUP_RETHROW_FALSE(w.u8(b));
			auto len = (uint16_t)children.size();
			SOUP_RETHROW_FALSE(w.u16_be(len));
		}
		else if (children.size() <= 0xffff'ffff)
		{
			uint8_t b = 0xdd;
			SOUP_RETHROW_FALSE(w.u8(b));
			auto len = (uint32_t)children.size();
			SOUP_RETHROW_FALSE(w.u32_be(len));
		}
		else
		{
			SOUP_ASSERT_UNREACHABLE;
		}

		for (const auto& child : children)
		{
			SOUP_RETHROW_FALSE(child->msgpackEncode(w));
		}

		return true;
	}

	JsonNode& JsonArray::at(size_t i) const
	{
		return *children.at(i);
	}

	void JsonArray::clear() noexcept
	{
		children.clear();
	}

	JsonArrayIterator JsonArray::begin() const noexcept
	{
		return { this, 0 };
	}

	JsonArrayIterator JsonArray::end() const noexcept
	{
		return { this, children.size() };
	}
}
