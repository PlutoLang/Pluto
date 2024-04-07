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

	JsonArray::JsonArray() noexcept
		: JsonNode(JSON_ARRAY)
	{
	}

	JsonArray::JsonArray(const char*& c)
		: JsonArray()
	{
		while (true)
		{
			json::handleLeadingSpace(c);
			auto val = json::decode(c);
			if (!val)
			{
				break;
			}
			children.emplace_back(std::move(val));
			while (*c == ',' || string::isSpace(*c))
			{
				++c;
			}
			if (*c == ']' || *c == 0)
			{
				break;
			}
		}
		++c;
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

	bool JsonArray::binaryEncode(Writer& w) const
	{
		{
			uint8_t b = JSON_ARRAY;
			if (!w.u8(b))
			{
				return false;
			}
		}

		for (const auto& child : children)
		{
			if (!child->binaryEncode(w))
			{
				return false;
			}
		}

		{
			uint8_t b = 0b111;
			if (!w.u8(b))
			{
				return false;
			}
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
