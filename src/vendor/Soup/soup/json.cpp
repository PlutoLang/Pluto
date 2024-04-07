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
	UniquePtr<JsonNode> json::decode(const std::string& data)
	{
		if (data.empty())
		{
			return {};
		}
		const char* c = data.c_str();
		return decode(c);
	}

	UniquePtr<JsonNode> json::decode(const char*& c)
	{
		handleLeadingSpace(c);

		switch (*c)
		{
		case '"':
			++c;
			return soup::make_unique<JsonString>(c);

		case '[':
			++c;
			return soup::make_unique<JsonArray>(c);

		case '{':
			++c;
			return soup::make_unique<JsonObject>(c);
		}

		std::string buf{};
		bool is_int = true;
		bool is_float = false;
		for (; *c != ',' && !string::isSpace(*c) && *c != '}' && *c != ']' && *c != ':' && *c != 0; ++c)
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
			++c;
			is_int = false;
			is_float = true;

			const bool negative = (*c == '-');
			if (!negative && *c != '+')
			{
				return {};
			}
			++c;

			for (; *c != ',' && !string::isSpace(*c) && *c != '}' && *c != ']' && *c != ':' && *c != 0; ++c)
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
				auto opt = string::toInt<int64_t>(buf);
				if (opt.has_value())
				{
					return soup::make_unique<JsonInt>(opt.value());
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
					return soup::make_unique<JsonFloat>(val);
				}
			}
			else if (buf == "true")
			{
				return soup::make_unique<JsonBool>(true);
			}
			else if (buf == "false")
			{
				return soup::make_unique<JsonBool>(false);
			}
			else if (buf == "null")
			{
				return soup::make_unique<JsonNull>();
			}
		}
		return {};
	}

	void json::decode(UniquePtr<JsonNode>& out, const std::string& data)
	{
		out = decode(data);
	}

	UniquePtr<JsonNode> json::decodeForDedicatedVariable(const std::string& data)
	{
		return decode(data);
	}

	void json::binaryDecode(UniquePtr<JsonNode>& out, Reader& r)
	{
		out = binaryDecodeForDedicatedVariable(r);
	}

	UniquePtr<JsonNode> json::binaryDecodeForDedicatedVariable(Reader& r)
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
				if (r.u64(val))
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

					if (binaryDecode(node, r), !node)
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

					if (binaryDecode(key, r), !key)
					{
						break;
					}
					if (binaryDecode(val, r), !val)
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

	void json::handleLeadingSpace(const char*& c)
	{
		while (*c != 0)
		{
			if (string::isSpace(*c))
			{
				++c;
			}
			else if (*c == '/')
			{
				handleComment(c);
			}
			else
			{
				break;
			}
		}
	}

	void json::handleComment(const char*& c)
	{
		++c;
		if (*c == '/')
		{
			do
			{
				++c;
			} while (*c != '\n' && *c != 0);
		}
		else if (*c == '*')
		{
			do
			{
				++c;
				if (*c == '*' && *(c + 1) == '/')
				{
					c += 2;
					break;
				}
			} while (*c != 0);
		}
		else
		{
			--c;
		}
	}
}
