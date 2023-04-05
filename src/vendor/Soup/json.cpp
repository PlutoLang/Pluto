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

namespace soup
{
	UniquePtr<JsonNode> json::decode(const std::string& data)
	{
		if (data.empty())
		{
			return {};
		}
		const char* c = &data.at(0);
		return decode(c);
	}

	UniquePtr<JsonNode> json::decode(const char*& c)
	{
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
			buf.push_back(*c);

			if (!string::isNumberChar(*c) && *c != '-')
			{
				is_int = false;
				is_float = (*c == '.');
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
				try
				{
					return soup::make_unique<JsonFloat>(std::stod(buf));
				}
				catch (...)
				{
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
}
