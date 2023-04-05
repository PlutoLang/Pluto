#include "JsonArray.hpp"

#include "json.hpp"
#include "string.hpp"
#include "Writer.hpp"

namespace soup
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
			while (string::isSpace(*c))
			{
				++c;
			}
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

	std::string JsonArray::encode() const
	{
		std::string res(1, '[');
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			res.append((*i)->encode());
			if (i != children.end() - 1)
			{
				res.push_back(',');
			}
		}
		res.push_back(']');
		return res;
	}

	std::string JsonArray::encodePretty(const std::string& prefix) const
	{
		if (children.empty())
		{
			return "[]";
		}
		std::string rprefix = prefix;
		rprefix.append("    ");
		std::string res = "[\n";
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			res.append(rprefix);
			res.append((*i)->encodePretty(rprefix));
			if (i != children.end() - 1)
			{
				res.push_back(',');
			}
			res.push_back('\n');
		}
		res.append(prefix).push_back(']');
		return res;
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

	JsonArrayIterator JsonArray::begin() const noexcept
	{
		return { this, 0 };
	}

	JsonArrayIterator JsonArray::end() const noexcept
	{
		return { this, children.size() };
	}
}
