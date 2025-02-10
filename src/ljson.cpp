#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "ljson.hpp"

#include "vendor/Soup/soup/string.hpp"

static void encodestring(lua_State* L, const char* data, size_t size, std::string& str)
{
#if !SOUP_LINUX
	str.reserve(str.size() + size + 2);
#endif
	str.push_back('"');
	for (size_t i = 0; i != size; ++i)
	{
		const char c = data[i];
		switch (c)
		{
		default:
			str.push_back(c);
			break;

		case 0x00: str.append("\\u0000"); break;
		case 0x01: str.append("\\u0001"); break;
		case 0x02: str.append("\\u0002"); break;
		case 0x03: str.append("\\u0003"); break;
		case 0x04: str.append("\\u0004"); break;
		case 0x05: str.append("\\u0005"); break;
		case 0x06: str.append("\\u0006"); break;
		case 0x07: str.append("\\u0007"); break;
		case 0x08: str.append("\\u0008"); break;
		case 0x09: str.append("\\t"); static_assert('\t' == 0x09); break;
		case 0x0A: str.append("\\n"); static_assert('\n' == 0x0A); break;
		case 0x0B: str.append("\\u000B"); break;
		case 0x0C: str.append("\\u000C"); break;
		case 0x0D: str.append("\\r"); static_assert('\r' == 0x0D); break;
		case 0x0E: str.append("\\u000E"); break;
		case 0x0F: str.append("\\u000F"); break;
		case 0x10: str.append("\\u0010"); break;
		case 0x11: str.append("\\u0011"); break;
		case 0x12: str.append("\\u0012"); break;
		case 0x13: str.append("\\u0013"); break;
		case 0x14: str.append("\\u0014"); break;
		case 0x15: str.append("\\u0015"); break;
		case 0x16: str.append("\\u0016"); break;
		case 0x17: str.append("\\u0017"); break;
		case 0x18: str.append("\\u0018"); break;
		case 0x19: str.append("\\u0019"); break;
		case 0x1A: str.append("\\u001A"); break;
		case 0x1B: str.append("\\u001B"); break;
		case 0x1C: str.append("\\u001C"); break;
		case 0x1D: str.append("\\u001D"); break;
		case 0x1E: str.append("\\u001E"); break;
		case 0x1F: str.append("\\u001F"); break;

		case '\\':
			str.append("\\\\");
			break;

		case '\"':
			str.append("\\\"");
			break;
		}
	}
	str.push_back('"');
}

static void encodeaux (lua_State *L, int i, bool pretty, std::string& str, unsigned depth = 0)
{
	switch (lua_type(L, i))
	{
	case LUA_TBOOLEAN:
		if (lua_toboolean(L, i))
		{
			str.append("true");
		}
		else
		{
			str.append("false");
		}
		break;

	case LUA_TNUMBER:
		if (lua_isinteger(L, i))
		{
			str.append(std::to_string(lua_tointeger(L, i)));
		}
		else
		{
			str.append(soup::string::fdecimal(lua_tonumber(L, i)));
		}
		break;

	case LUA_TSTRING: {
		size_t size;
		const char* data = luaL_checklstring(L, i, &size);
		encodestring(L, data, size, str);
	} break;

	case LUA_TTABLE: {
		lua_checkstack(L, 5);
		lua_pushvalue(L, i);
		const auto child_depth = (depth + 1);
		if (isIndexBasedTable(L, -1))
		{
			str.push_back('[');
			lua_pushnil(L);
			while (lua_next(L, -2))
			{
				lua_pushvalue(L, -2);
				if (pretty)
				{
					str.push_back('\n');
					str.append(child_depth * 4, ' ');
				}
				{
					luaE_incCstack(L);
					encodeaux(L, -2, pretty, str, child_depth);
					L->nCcalls--;
				}
				str.push_back(',');
				lua_pop(L, 2);
			}
			if (str.back() == ',')
			{
				str.pop_back();
				if (pretty)
				{
					str.push_back('\n');
					str.append(depth * 4, ' ');
				}
			}
			str.push_back(']');
		}
		else
		{
			str.push_back('{');
			lua_pushliteral(L, "__order");
			if (lua_rawget(L, -2) == LUA_TTABLE)
			{
				// table, __order
				lua_pushnil(L);
				// table, __order, idx
				while (lua_next(L, -2))
				{
					// table, __order, idx, key
					lua_pushvalue(L, -1);
					// table, __order, idx, key, key
					if (lua_rawget(L, -5) > LUA_TNIL)
					{
						// table, __order, idx, key, value
						if (pretty)
						{
							str.push_back('\n');
							str.append(child_depth * 4, ' ');
						}
						luaE_incCstack(L);
						encodeaux(L, -2, pretty, str, child_depth);
						str.push_back(':');
						if (pretty)
						{
							str.push_back(' ');
						}
						encodeaux(L, -1, pretty, str, child_depth);
						L->nCcalls--;
						str.push_back(',');
					}
					// table, __order, idx, key, value
					lua_pop(L, 2);
					// table, __order, idx
				}
				lua_pop(L, 1);  /* pop __order */
			}
			else
			{
				lua_pop(L, 1);  /* pop __order (nil)*/
				lua_pushnil(L);
				while (lua_next(L, -2))
				{
					lua_pushvalue(L, -2);
					if (pretty)
					{
						str.push_back('\n');
						str.append(child_depth * 4, ' ');
					}
					luaE_incCstack(L);
					encodeaux(L, -1, pretty, str, child_depth);
					str.push_back(':');
					if (pretty)
					{
						str.push_back(' ');
					}
					encodeaux(L, -2, pretty, str, child_depth);
					L->nCcalls--;
					str.push_back(',');
					lua_pop(L, 2);
				}
			}
			if (str.back() == ',')
			{
				str.pop_back();
				if (pretty)
				{
					str.push_back('\n');
					str.append(depth * 4, ' ');
				}
			}
			str.push_back('}');
		}
		lua_pop(L, 1);
	} break;

	case LUA_TLIGHTUSERDATA:
		if (reinterpret_cast<uintptr_t>(lua_touserdata(L, i)) == 0xF01D)
		{
			str.append("null");
			break;
		}
		[[fallthrough]];
	default:
		luaL_typeerror(L, i, "JSON-castable type");
	}
}

static int encode(lua_State* L) {
	std::string str;
	encodeaux(L, 1, lua_istrue(L, 2), str);
	pluto_pushstring(L, str);
	return 1;
}

static int decode(lua_State* L)
{
	int flags = (int)luaL_optinteger(L, 2, 0);
	soup::UniquePtr<soup::JsonNode> root;
	try
	{
		root = soup::json::decode(pluto_checkstring(L, 1));
	}
	catch (const std::exception& e)
	{
		luaL_error(L, e.what());
	}
	if (root)
	{
		pushFromJson(L, *root, flags);
		return 1;
	}
	return 0;
}

static const luaL_Reg funcs[] = {
	{"encode", encode},
	{"decode", decode},
	{nullptr, nullptr}
};

LUAMOD_API int luaopen_json(lua_State* L)
{
	luaL_newlib(L, funcs);
	lua_pushlightuserdata(L, reinterpret_cast<void*>(static_cast<uintptr_t>(0xF01D)));
	lua_setfield(L, -2, "null");

	// decode flags
	lua_pushinteger(L, 1 << 0);
	lua_setfield(L, -2, "withnull");
	lua_pushinteger(L, 1 << 1);
	lua_setfield(L, -2, "withorder");

	return 1;
}
const Pluto::PreloadedLibrary Pluto::preloaded_json{ "json", funcs, &luaopen_json };
