#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "ljson.hpp"

#include "vendor/Soup/soup/string.hpp"

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
			lua_Number n = lua_tonumber(L, i);
			if (std::isfinite(n))
			{
				str.append(soup::string::fdecimal(n));
				return;
			}
			luaL_error(L, "%f has no JSON representation", n);
		}
		break;

	case LUA_TSTRING: {
		size_t size;
		const char* data = luaL_checklstring(L, i, &size);
		soup::JsonString::encodeValue(str, data, size);
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
	auto str = pluto_newclassinst(L, std::string);
	encodeaux(L, 1, lua_istrue(L, 2), *str);
	pluto_pushstring(L, *str);
	return 1;
}

static int decode(lua_State* L)
{
	size_t size;
	const char* data = luaL_checklstring(L, 1, &size);
	int flags = (int)luaL_optinteger(L, 2, 0);
	lua_checkstack(L, 1);
	soup::JsonTreeWriter jtw;
	jtw.allocArray = [](void* L, size_t reserve_size) -> void* {
		lua_checkstack((lua_State*)L, 3);
		lua_createtable((lua_State*)L, (unsigned int)reserve_size, 0);
		lua_pushinteger((lua_State*)L, 1);
		return L; // must not return nullptr
	};
	jtw.addToArray = [](void* L, void* array, void* value) -> void {
		const auto k = lua_tointeger((lua_State*)L, -2);
		lua_settable((lua_State*)L, -3);
		lua_pushinteger((lua_State*)L, k + 1);
	};
	jtw.onArrayFinished = [](void* L, void* array) -> void {
		lua_pop((lua_State*)L, 1);
	};
	if (flags & (1 << 1)) // json.withorder
	{
		jtw.allocObject = [](void* L, size_t reserve_size) -> void* {
			lua_checkstack((lua_State*)L, 7);
			lua_createtable((lua_State*)L, 0, (unsigned int)reserve_size);
			// stack now: table
			lua_createtable((lua_State*)L, (unsigned int)reserve_size, 0);
			// stack now: table, order table
			lua_pushliteral((lua_State*)L, "__order");
			lua_pushvalue((lua_State*)L, -2);
			// stack now: table, order table, "__order", order table
			lua_settable((lua_State*)L, -4);
			// stack now: table, order table
			lua_pushinteger((lua_State*)L, 1);
			// stack now: table, order table, order index
			return L; // must not return nullptr
		};
		jtw.addToObject = [](void* L, void* object, void* key, void* value) -> void {
			// stack now: table, order table, order index, key, value
			lua_pushvalue((lua_State*)L, -2);
			lua_pushvalue((lua_State*)L, -2);
			// stack now: table, order table, order index, key, value, key, value
			lua_settable((lua_State*)L, -7);
			// stack now: table, order table, order index, key, value
			lua_pop((lua_State*)L, 1);
			// stack now: table, order table, order index, key
			const auto k = lua_tointeger((lua_State*)L, -2);
			lua_settable((lua_State*)L, -3);
			// stack now: table, order table
			lua_pushinteger((lua_State*)L, k + 1);
			// stack now: table, order table, order index
		};
		jtw.onObjectFinished = [](void* L, void* object) -> void {
			// stack now: table, order table, order index
			lua_pop((lua_State*)L, 2);
			// stack now: table
		};
	}
	else
	{
		jtw.allocObject = [](void* L, size_t reserve_size) -> void* {
			lua_checkstack((lua_State*)L, 3);
			lua_createtable((lua_State*)L, 0, (unsigned int)reserve_size);
			return L; // must not return nullptr
		};
		jtw.addToObject = [](void* L, void* object, void* key, void* value) -> void {
			lua_settable((lua_State*)L, -3);
		};
	}
	jtw.allocString = [](void* L, std::string&& value) -> void* {
		pluto_pushstring((lua_State*)L, value);
		return L; // must not return nullptr
	};
	jtw.allocUnescapedString = [](void* L, const char* data, size_t size) -> void* {
		lua_pushlstring((lua_State*)L, data, size);
		return L; // must not return nullptr
	};
	jtw.allocInt = [](void* L, int64_t value) -> void* {
		lua_pushinteger((lua_State*)L, value);
		return L; // must not return nullptr
	};
	jtw.allocFloat = [](void* L, double value) -> void* {
		lua_pushnumber((lua_State*)L, value);
		return L; // must not return nullptr
	};
	jtw.allocBool = [](void* L, bool value) -> void* {
		lua_pushboolean((lua_State*)L, value);
		return L; // must not return nullptr
	};
	if (flags & (1 << 0)) // json.withnull
	{
		jtw.allocNull = [](void* L) -> void* {
			lua_pushlightuserdata((lua_State*)L, reinterpret_cast<void*>(static_cast<uintptr_t>(0xF01D)));
			return L; // must not return nullptr
		};
	}
	else
	{
		jtw.allocNull = [](void* L) -> void* {
			lua_pushnil((lua_State*)L);
			return L; // must not return nullptr
		};
	}
	jtw.free = [](void* L, void* node) -> void {
		lua_pop((lua_State*)L, 1);
	};
	try
	{
		if (soup::json::decode(jtw, (void*)L, data, size, 100))
		{
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		luaL_error(L, e.what());
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
