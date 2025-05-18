#include "json.hpp"

#include "JsonArray.hpp"
#include "JsonBool.hpp"
#include "JsonFloat.hpp"
#include "JsonInt.hpp"
#include "JsonNull.hpp"
#include "JsonObject.hpp"
#include "JsonString.hpp"
#include "Reader.hpp"
#include "string.hpp"

NAMESPACE_SOUP
{
	UniquePtr<JsonNode> json::decode(const char* data, size_t size, int max_depth)
	{
		JsonTreeWriter jtw;
		jtw.allocArray = [](void*) -> void* { return new JsonArray(); };
		jtw.allocObject = [](void*) -> void* { return new JsonObject(); };
		jtw.allocString = [](void*, std::string&& value) -> void* { return new JsonString(std::move(value)); };
		jtw.allocInt = [](void*, int64_t value) -> void* { return new JsonInt(value); };
		jtw.allocFloat = [](void*, double value) -> void* { return new JsonFloat(value); };
		jtw.allocBool = [](void*, bool value) -> void* { return new JsonBool(value); };
		jtw.allocNull = [](void*) -> void* { return new JsonNull(); };
		jtw.addToArray = [](void*, void* arr, void* value) -> void { ((JsonArray*)arr)->children.emplace_back((JsonNode*)value); };
		jtw.addToObject = [](void*, void* obj, void* key, void* value) -> void { ((JsonObject*)obj)->children.emplace_back((JsonNode*)key, (JsonNode*)value); };
		jtw.free = [](void*, void* node) -> void { delete (JsonNode*)node; };
		return (JsonNode*)decode(jtw, nullptr, data, size, max_depth);
	}

	void* json::decode(const JsonTreeWriter& tw, void* user_data, const char*& c, size_t& s, int max_depth)
	{
		SOUP_ASSERT(max_depth-- != 0, "Depth limit exceeded");

		handleLeadingSpace(c, s);

		switch (s != 0 ? *c : 0)
		{
		case '"':
			++c; --s;
			return tw.allocString(user_data, JsonString::decodeValue(c, s));

		case '[': {
			++c; --s;
			auto arr = tw.allocArray(user_data);
			while (true)
			{
				handleLeadingSpace(c, s);
				auto val = decode(tw, user_data, c, s, max_depth);
				SOUP_IF_UNLIKELY (!val)
				{
					break;
				}
				tw.addToArray(user_data, arr, val);
				while (s != 0 && (*c == ',' || string::isSpace(*c)))
				{
					++c; --s;
				}
				if (s == 0 || *c == ']')
				{
					break;
				}
			}
			if (tw.onArrayFinished)
			{
				tw.onArrayFinished(user_data, arr);
			}
			SOUP_IF_LIKELY (s != 0)
			{
				++c; --s;
			}
			return arr;
		}

		case '{': {
			++c; --s;
			auto obj = tw.allocObject(user_data);
			while (true)
			{
				handleLeadingSpace(c, s);
				if (s == 0 || *c == '}')
				{
					break;
				}
				auto key = decode(tw, user_data, c, s, max_depth);
				while (s != 0 && (string::isSpace(*c) || *c == ':'))
				{
					++c; --s;
				}
				auto val = decode(tw, user_data, c, s, max_depth);
				SOUP_IF_UNLIKELY (!key || !val)
				{
					if (val)
					{
						tw.free(user_data, val);
					}
					if (key)
					{
						tw.free(user_data, key);
					}
					break;
				}
				tw.addToObject(user_data, obj, key, val);
				while (s != 0 && (*c == ',' || string::isSpace(*c)))
				{
					++c; --s;
				}
			}
			if (tw.onObjectFinished)
			{
				tw.onObjectFinished(user_data, obj);
			}
			SOUP_IF_LIKELY (s != 0)
			{
				++c; --s;
			}
			return obj;
		}
		}

		std::string buf{};
		bool is_int = true;
		bool is_float = false;
		for (; s != 0 && *c != ',' && !string::isSpace(*c) && *c != '}' && *c != ']' && *c != ':'; ++c, --s)
		{
			if ((is_int || is_float) && (*c == 'e' || *c == 'E'))
			{
				break;
			}

			buf.push_back(*c);

			if (!string::isNumberChar(*c) && *c != '-')
			{
				is_int = false;
				is_float = (*c == '.');
			}
		}
		int exponent = 0;
		if (*c == 'e' || *c == 'E')
		{
			++c; --s;
			is_int = false;
			is_float = true;

			const bool negative = (*c == '-');
			if (!negative && *c != '+')
			{
				return {};
			}
			++c; --s;

			for (; s != 0 && *c != ',' && !string::isSpace(*c) && *c != '}' && *c != ']' && *c != ':'; ++c, --s)
			{
				exponent *= 10;
				exponent += ((*c) - '0');
			}
			if (negative)
			{
				exponent *= -1;
			}
		}
		if (!buf.empty())
		{
			if (is_int)
			{
				auto opt = string::toIntOpt<int64_t>(buf);
				if (opt.has_value())
				{
					return tw.allocInt(user_data, opt.value());
				}
			}
			else if (is_float)
			{
				char* str_end;
				auto val = std::strtod(buf.c_str(), &str_end);
				if (str_end != buf.c_str() && val != HUGE_VAL)
				{
					if (exponent != 0)
					{
						val *= std::pow(10.0, exponent);
					}
					return tw.allocFloat(user_data, val);
				}
			}
			else if (buf == "true")
			{
				return tw.allocBool(user_data, true);
			}
			else if (buf == "false")
			{
				return tw.allocBool(user_data, false);
			}
			else if (buf == "null")
			{
				return tw.allocNull(user_data);
			}
		}
		return nullptr;
	}

	UniquePtr<JsonNode> json::binaryDecode(Reader& r)
	{
		uint8_t b;
		if (r.u8(b))
		{
			uint8_t type = (b & 0b111);
			if (type == JSON_INT)
			{
				uint8_t extra = (b >> 3);
				int64_t val;
				if (extra == 0b11111
					? r.i64_dyn(val)
					: (val = extra, true)
					)
				{
					return soup::make_unique<JsonInt>(val);
				}
			}
			else if (type == JSON_FLOAT)
			{
				uint64_t val;
				if (r.u64le(val))
				{
					return soup::make_unique<JsonFloat>(*reinterpret_cast<double*>(&val));
				}
			}
			else if (type == JSON_STRING)
			{
				uint8_t len = (b >> 3);
				std::string val;
				if (len == 0b11111
					? r.str_lp_u64_dyn(val)
					: r.str(len, val)
					)
				{
					return soup::make_unique<JsonString>(std::move(val));
				}
			}
			else if (type == JSON_BOOL)
			{
				return soup::make_unique<JsonBool>(b >> 3);
			}
			else if (type == JSON_NULL)
			{
				return soup::make_unique<JsonNull>();
			}
			else if (type == JSON_ARRAY)
			{
				auto arr = soup::make_unique<JsonArray>();
				while (true)
				{
					UniquePtr<JsonNode> node;

					if (node = binaryDecode(r), !node)
					{
						break;
					}

					arr->children.emplace_back(std::move(node));
				}
				return arr;
			}
			else if (type == JSON_OBJECT)
			{
				auto obj = soup::make_unique<JsonObject>();
				while (true)
				{
					UniquePtr<JsonNode> key;
					UniquePtr<JsonNode> val;

					if (key = binaryDecode(r), !key)
					{
						break;
					}
					if (val = binaryDecode(r), !val)
					{
						break;
					}

					obj->children.emplace_back(std::move(key), std::move(val));
				}
				return obj;
			}
		}
		return {};
	}

	void json::handleLeadingSpace(const char*& c, size_t& s)
	{
		while (s != 0)
		{
			if (string::isSpace(*c))
			{
				++c; --s;
			}
			else if (*c == '/')
			{
				handleComment(c, s);
			}
			else
			{
				break;
			}
		}
	}

	void json::handleComment(const char*& c, size_t& s)
	{
		++c; --s;
		if (*c == '/')
		{
			do
			{
				++c; --s;
			} while (*c != '\n' && *c != 0);
		}
		else if (*c == '*')
		{
			do
			{
				++c; --s;
				if (*c == '*' && *(c + 1) == '/')
				{
					c += 2;
					s -= 2;
					break;
				}
			} while (s != 0);
		}
		else
		{
			--c; ++s;
		}
	}
}
