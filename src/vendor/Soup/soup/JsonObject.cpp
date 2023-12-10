#include "JsonObject.hpp"

#include "Exception.hpp"
#include "json.hpp"
#include "JsonBool.hpp"
#include "JsonInt.hpp"
#include "JsonString.hpp"
#include "string.hpp"
#include "Writer.hpp"

namespace soup
{
	JsonObject::JsonObject() noexcept
		: JsonNode(JSON_OBJECT)
	{
	}

	JsonObject::JsonObject(const char*& c) noexcept
		: JsonObject()
	{
		while (true)
		{
			while (string::isSpace(*c))
			{
				++c;
			}
			if (*c == '}' || *c == 0)
			{
				break;
			}
			auto key = json::decode(c);
			while (string::isSpace(*++c));
			auto val = json::decode(c);
			if (!key || !val)
			{
				break;
			}
			children.emplace_back(std::move(key), std::move(val));
			while (*c == ',' || string::isSpace(*c))
			{
				++c;
			}
		}
		++c;
	}

	std::string JsonObject::encode() const
	{
		std::string res(1, '{');
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			res.append(i->first->encode());
			res.push_back(':');
			res.append(i->second->encode());
			if (i != children.end() - 1)
			{
				res.push_back(',');
			}
		}
		res.push_back('}');
		return res;
	}

	std::string JsonObject::encodePretty(const std::string& prefix) const
	{
		if (children.empty())
		{
			return "{}";
		}
		std::string rprefix = prefix;
		rprefix.append("    ");
		std::string res = "{\n";
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			res.append(rprefix);
			res.append(i->first->encode());
			res.append(": ");
			res.append(i->second->encodePretty(rprefix));
			if (i != children.end() - 1)
			{
				res.push_back(',');
			}
			res.push_back('\n');
		}
		res.append(prefix).push_back('}');
		return res;
	}

	bool JsonObject::binaryEncode(Writer& w) const
	{
		{
			uint8_t b = JSON_OBJECT;
			if (!w.u8(b))
			{
				return false;
			}
		}

		for (const auto& child : children)
		{
			if (!child.first->binaryEncode(w)
				|| !child.second->binaryEncode(w)
				)
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

	UniquePtr<JsonNode>* JsonObject::findUp(const JsonNode& k) noexcept
	{
		for (auto& child : children)
		{
			if (*child.first == k)
			{
				return &child.second;
			}
		}
		return nullptr;
	}

	JsonNode* JsonObject::find(const JsonNode& k) const noexcept
	{
		for (const auto& child : children)
		{
			if (*child.first == k)
			{
				return child.second.get();
			}
		}
		return nullptr;
	}

	JsonNode* JsonObject::find(std::string k) const noexcept
	{
		return find(JsonString(std::move(k)));
	}

	bool JsonObject::contains(const JsonNode& k) const noexcept
	{
		return find(k) != nullptr;
	}

	bool JsonObject::contains(std::string k) const noexcept
	{
		return contains(JsonString(std::move(k)));
	}

	JsonNode& JsonObject::at(const JsonNode& k) const
	{
		if (auto e = find(k))
		{
			return *e;
		}
		std::string err = "JsonObject has no member with key ";
		err.append(k.encode());
		throw Exception(std::move(err));
	}

	JsonNode& JsonObject::at(std::string k) const
	{
		return at(JsonString(std::move(k)));
	}

	void JsonObject::erase(const JsonNode& k) noexcept
	{
		for (auto i = children.begin(); i != children.end(); ++i)
		{
			if (*i->first == k)
			{
				children.erase(i);
				break;
			}
		}
	}

	void JsonObject::erase(std::string k) noexcept
	{
		return erase(JsonString(std::move(k)));
	}

	void JsonObject::add(UniquePtr<JsonNode>&& k, UniquePtr<JsonNode>&& v)
	{
		children.emplace_back(std::move(k), std::move(v));
	}

	void JsonObject::add(std::string k, std::string v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonString>(std::move(v)));
	}

	void JsonObject::add(std::string k, const char* v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonString>(v));
	}

	void JsonObject::add(std::string k, int8_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int16_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int32_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, int64_t v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonInt>(v));
	}

	void JsonObject::add(std::string k, bool v)
	{
		add(soup::make_unique<JsonString>(std::move(k)), soup::make_unique<JsonBool>(v));
	}
}
