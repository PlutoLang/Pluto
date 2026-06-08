#include "json.hpp"

#include "filesystem.hpp"
#include "Reader.hpp"
#include "string.hpp"
#include "time.hpp"

NAMESPACE_SOUP
{
	UniquePtr<JsonNode> json::decode(const char* data, size_t size, int max_depth)
	{
		DefaultJsonTreeWriter jtw;
		return (JsonNode*)decode(jtw, nullptr, data, size, max_depth);
	}

	UniquePtr<JsonNode> json::decodeFile(const std::filesystem::path& path, int max_depth)
	{
#if !SOUP_WASM || SOUP_EMSCRIPTEN
		UniquePtr<JsonNode> res;
		size_t size;
		if (auto data = filesystem::createFileMapping(path, size))
		{
			res = json::decode((const char*)data, size, max_depth);
			filesystem::destroyFileMapping(data, size);
		}
		return res;
#else
		return json::decode(string::fromFile(path), max_depth);
#endif
	}

	void* json::decode(const JsonTreeWriter& tw, void* user_data, const char*& c, size_t& s, int max_depth)
	{
		SOUP_ASSERT(max_depth-- != 0, "Depth limit exceeded");

		handleLeadingSpace(c, s);

		switch (s != 0 ? *c : 0)
		{
		case '"': {
			++c; --s;
			const auto encoded_size = JsonString::getEncodedSize(c, s);
			if (std::string_view(c, encoded_size).find('\\') == std::string::npos)
			{
				const auto str = tw.allocUnescapedString(user_data, c, encoded_size);
				c += encoded_size;
				s -= encoded_size;
				SOUP_IF_LIKELY (s != 0)
				{
					++c; --s;
				}
				return str;
			}
			else
			{
				std::string value;
				value.reserve(encoded_size);
				JsonString::decodeValue(value, c, s);
				value.shrink_to_fit();
				return tw.allocString(user_data, std::move(value));
			}
		}

		case '[': {
			++c; --s;
			auto arr = tw.allocArray(user_data, 0);
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
			auto obj = tw.allocObject(user_data, 0);
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

	UniquePtr<JsonNode> json::msgpackDecode(Reader& r, int max_depth)
	{
		DefaultJsonTreeWriter jtw;
		return (JsonNode*)msgpackDecode(jtw, nullptr, r, max_depth);
	}

	void* json::msgpackDecode(const JsonTreeWriter& tw, void* user_data, Reader& r, int max_depth)
	{
		SOUP_ASSERT(max_depth-- != 0, "Depth limit exceeded");

		uint8_t b;
		SOUP_RETHROW_FALSE(r.u8(b));

		// nil:
		// - [x] 0xc0
		// bool:
		// - [x] 0xc2
		// - [x] 0xc3
		// int:
		// - [x] 0XXXXXXX
		// - [x] 111YYYYY
		// - [x] 0xcc
		// - [x] 0xcd
		// - [x] 0xce
		// - [x] 0xcf
		// - [x] 0xd0
		// - [x] 0xd1
		// - [x] 0xd2
		// - [x] 0xd3
		// float:
		// - [x] 0xca
		// - [x] 0xcb
		// str:
		// - [x] 101XXXXX
		// - [x] 0xd9
		// - [x] 0xda
		// - [x] 0xdb
		// array:
		// - [x] 1001XXXX
		// - [x] 0xdc
		// - [x] 0xdd
		// map:
		// - [x] 1000XXXX
		// - [x] 0xde
		// - [x] 0xdf

		// Bit 7 not set -> unsigned int
		if (!((b >> 7) & 1))
		{
			return tw.allocInt(user_data, b);
		}

		if ((b >> 6) & 1) // Bit 6 set?
		{
			// Bit 5 set -> signed int
			if ((b >> 5) & 1)
			{
				return tw.allocInt(user_data, static_cast<int8_t>(b));
			}

			// Bit 5 not set -> type id
			switch (b)
			{
			case 0xc0:
				return tw.allocNull(user_data);

			case 0xc2:
				return tw.allocBool(user_data, false);

			case 0xc3:
				return tw.allocBool(user_data, true);

			case 0xca: {
				float val;
				SOUP_RETHROW_FALSE(r.f32(val));
				return tw.allocFloat(user_data, val);
			}

			case 0xcb: {
				double val;
				SOUP_RETHROW_FALSE(r.f64(val));
				return tw.allocFloat(user_data, val);
			}

			case 0xcc: {
				uint8_t val;
				SOUP_RETHROW_FALSE(r.u8(val));
				return tw.allocInt(user_data, val);
			}

			case 0xcd: {
				uint16_t val;
				SOUP_RETHROW_FALSE(r.u16_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xce: {
				uint32_t val;
				SOUP_RETHROW_FALSE(r.u32_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xcf: {
				uint64_t val;
				SOUP_RETHROW_FALSE(r.u64_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xd0: {
				int8_t val;
				SOUP_RETHROW_FALSE(r.i8(val));
				return tw.allocInt(user_data, val);
			}

			case 0xd1: {
				int16_t val;
				SOUP_RETHROW_FALSE(r.i16_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xd2: {
				int32_t val;
				SOUP_RETHROW_FALSE(r.i32_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xd3: {
				int64_t val;
				SOUP_RETHROW_FALSE(r.i64_be(val));
				return tw.allocInt(user_data, val);
			}

			case 0xd9: {
				uint8_t len;
				SOUP_RETHROW_FALSE(r.u8(len));
				if (auto data = r.getMemoryView(len))
				{
					r.skip(len);
					return tw.allocUnescapedString(user_data, (const char*)data, len);
				}
				std::string data;
				SOUP_RETHROW_FALSE(r.str(len, data));
				return tw.allocString(user_data, std::move(data));
			}

			case 0xda: {
				uint16_t len;
				SOUP_RETHROW_FALSE(r.u16_be(len));
				if (auto data = r.getMemoryView(len))
				{
					r.skip(len);
					return tw.allocUnescapedString(user_data, (const char*)data, len);
				}
				std::string data;
				SOUP_RETHROW_FALSE(r.str(len, data));
				return tw.allocString(user_data, std::move(data));
			}

			case 0xdb: {
				uint32_t len;
				SOUP_RETHROW_FALSE(r.u32_be(len));
				if (auto data = r.getMemoryView(len))
				{
					r.skip(len);
					return tw.allocUnescapedString(user_data, (const char*)data, len);
				}
				std::string data;
				SOUP_RETHROW_FALSE(r.str(len, data));
				return tw.allocString(user_data, std::move(data));
			}

			case 0xdc: {
				uint16_t len;
				SOUP_RETHROW_FALSE(r.u16_be(len));
				auto arr = tw.allocArray(user_data, len);
				while (len--)
				{
					void* node = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!node)
					{
						tw.free(user_data, arr);
						return nullptr;
					}
					tw.addToArray(user_data, arr, node);
				}
				if (tw.onArrayFinished)
				{
					tw.onArrayFinished(user_data, arr);
				}
				return arr;
			}

			case 0xdd: {
				uint32_t len;
				SOUP_RETHROW_FALSE(r.u32_be(len));
				auto arr = tw.allocArray(user_data, len);
				while (len--)
				{
					void* node = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!node)
					{
						tw.free(user_data, arr);
						return nullptr;
					}
					tw.addToArray(user_data, arr, node);
				}
				if (tw.onArrayFinished)
				{
					tw.onArrayFinished(user_data, arr);
				}
				return arr;
			}

			case 0xde: {
				uint16_t len;
				SOUP_RETHROW_FALSE(r.u16_be(len));
				auto obj = tw.allocObject(user_data, len);
				while (len--)
				{
					void* key = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!key)
					{
						tw.free(user_data, obj);
						return nullptr;
					}
					void* val = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!val)
					{
						tw.free(user_data, key);
						tw.free(user_data, obj);
						return nullptr;
					}
					tw.addToObject(user_data, obj, key, val);
				}
				if (tw.onObjectFinished)
				{
					tw.onObjectFinished(user_data, obj);
				}
				return obj;
			}

			case 0xdf: {
				uint32_t len;
				SOUP_RETHROW_FALSE(r.u32_be(len));
				auto obj = tw.allocObject(user_data, len);
				while (len--)
				{
					void* key = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!key)
					{
						tw.free(user_data, obj);
						return nullptr;
					}
					void* val = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!val)
					{
						tw.free(user_data, key);
						tw.free(user_data, obj);
						return nullptr;
					}
					tw.addToObject(user_data, obj, key, val);
				}
				if (tw.onObjectFinished)
				{
					tw.onObjectFinished(user_data, obj);
				}
				return obj;
			}
			}
		}
		else
		{
			// Bit 5 set -> str
			if ((b >> 5) & 1)
			{
				uint8_t len = b & 0b11111;
				if (auto data = r.getMemoryView(len))
				{
					r.skip(len);
					return tw.allocUnescapedString(user_data, (const char*)data, len);
				}
				std::string data;
				SOUP_RETHROW_FALSE(r.str(len, data));
				return tw.allocString(user_data, std::move(data));
			}

			// Array or map
			uint8_t len = b & 0b1111;
			if ((b >> 4) & 1) // Bit 4 set -> array
			{
				auto arr = tw.allocArray(user_data, len);
				while (len--)
				{
					void* node = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!node)
					{
						tw.free(user_data, arr);
						return nullptr;
					}
					tw.addToArray(user_data, arr, node);
				}
				if (tw.onArrayFinished)
				{
					tw.onArrayFinished(user_data, arr);
				}
				return arr;
			}
			else // Bit 4 not set -> map
			{
				auto obj = tw.allocObject(user_data, len);
				while (len--)
				{
					void* key = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!key)
					{
						tw.free(user_data, obj);
						return nullptr;
					}
					void* val = msgpackDecode(tw, user_data, r, max_depth);
					SOUP_IF_UNLIKELY (!val)
					{
						tw.free(user_data, key);
						tw.free(user_data, obj);
						return nullptr;
					}
					tw.addToObject(user_data, obj, key, val);
				}
				if (tw.onObjectFinished)
				{
					tw.onObjectFinished(user_data, obj);
				}
				return obj;
			}
		}

		return {};
	}

	UniquePtr<JsonObject> json::bsonDecode(Reader& r, int max_depth)
	{
		SOUP_ASSERT(max_depth-- != 0, "Depth limit exceeded");

		auto obj = soup::make_unique<JsonObject>();
		r.skip(4); // length prefix
		while (true)
		{
			uint8_t k;
			r.u8(k);
			switch (k)
			{
			case 0:
				return obj;

			case 1:
				{
					std::string n; r.str_nt(n);
					double v; r.f64(v);
					obj->add(std::move(n), v);
				}
				break;

			case 2:
				{
					std::string n; r.str_nt(n);
					uint32_t len; r.u32_le(len);
					SOUP_RETHROW_FALSE(len != 0);
					len -= 1; // null terminator
					std::string v; r.str(len, v);
					obj->add(std::move(n), std::move(v));
					r.skip(1); // null terminator
				}
				break;

			case 3:
				{
					std::string n; r.str_nt(n);
					auto v = bsonDecode(r, max_depth);
					SOUP_RETHROW_FALSE(v);
					obj->add(std::move(n), std::move(v));
				}
				break;

			case 4:
				{
					std::string n; r.str_nt(n);
					auto v = bsonDecode(r, max_depth);
					SOUP_RETHROW_FALSE(v);
					auto arr = soup::make_unique<JsonArray>();
					for (auto& e : v->children)
					{
						arr->children.emplace_back(std::move(e.second));
					}
					obj->add(std::move(n), std::move(arr));
				}
				break;

			case 7:
				{
					std::string n; r.str_nt(n);
					char v[12]; r.raw(v, sizeof(v));
					auto w = soup::make_unique<JsonObject>();
					w->add("$oid", string::bin2hexLower(v, sizeof(v)));
					obj->add(std::move(n), std::move(w));
				}
				break;

			case 8:
				{
					std::string n; r.str_nt(n);
					bool v; r.b(v);
					obj->add(std::move(n), v);
				}
				break;

			case 9:
				{
					std::string n; r.str_nt(n);
					int64_t v; r.i64_le(v);
					auto w = soup::make_unique<JsonObject>();
					w->add("$date", time::toIso8601(v / 1000)); // TODO: Append .123 to avoid precision loss?
					obj->add(std::move(n), std::move(w));
				}
				break;

			case 10:
				{
					std::string n; r.str_nt(n);
					obj->add(std::move(n), soup::make_unique<JsonNull>());
				}
				break;

			case 16:
				{
					std::string n; r.str_nt(n);
					int32_t v; r.i32_le(v);
					obj->add(std::move(n), v);
				}
				break;

			case 18:
				{
					std::string n; r.str_nt(n);
					int64_t v; r.i64_le(v);
					obj->add(std::move(n), v);
				}
				break;

			default:
				//std::cout << "Unhandled BSON type: " << (int)k << std::endl;
				return {};
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
