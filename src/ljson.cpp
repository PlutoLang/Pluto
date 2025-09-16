#define LUA_LIB

#include <algorithm>
#include <cmath> // isfinite

#include "lauxlib.h"
#include "lualib.h"
#include "ljson.hpp" // isIndexBasedTable
#include "lstate.h" // luaE_incCstack

#include "vendor/Soup/soup/json.hpp"
#include "vendor/Soup/soup/JsonArray.hpp"
#include "vendor/Soup/soup/JsonBool.hpp"
#include "vendor/Soup/soup/JsonFloat.hpp"
#include "vendor/Soup/soup/JsonInt.hpp"
#include "vendor/Soup/soup/JsonNode.hpp"
#include "vendor/Soup/soup/JsonObject.hpp"
#include "vendor/Soup/soup/JsonString.hpp"
#include "vendor/Soup/soup/MemoryRefReader.hpp"
#include "vendor/Soup/soup/string.hpp"
#include "vendor/Soup/soup/StringWriter.hpp"
#include "vendor/Soup/soup/UniquePtr.hpp"


static void checkJson(lua_State* L, int i, soup::UniquePtr<soup::JsonNode>& out)
{
	const auto type = lua_type(L, i);
	if (type == LUA_TBOOLEAN)
	{
		out = soup::make_unique<soup::JsonBool>(lua_toboolean(L, i));
		return;
	}
	else if (type == LUA_TNUMBER)
	{
		if (lua_isinteger(L, i))
		{
			out = soup::make_unique<soup::JsonInt>(lua_tointeger(L, i));
			return;
		}
		else
		{
			lua_Number n = lua_tonumber(L, i);
			if (std::isfinite(n))
			{
				out = soup::make_unique<soup::JsonFloat>(n);
				return;
			}
			luaL_error(L, "%f has no JSON representation", n);
		}
	}
	else if (type == LUA_TSTRING)
	{
		out = soup::make_unique<soup::JsonString>(pluto_checkstring(L, i));
		return;
	}
	else if (type == LUA_TTABLE)
	{
		lua_checkstack(L, 5);
		if (const auto n = isIndexBasedTable(L, i))
		{
			out = soup::make_unique<soup::JsonArray>();
			auto& arr = out->reinterpretAsArr();
			for (lua_Integer k = 1; k != n; ++k)
			{
				if (l_unlikely(lua_geti(L, i, k) == LUA_TNIL))
				{
					lua_pop(L, 1);
					goto _encode_as_object;
				}
				luaE_incCstack(L);
				auto& entry = arr.children.emplace_back();
				checkJson(L, -1, entry);
				L->nCcalls--;
				lua_pop(L, 1);
			}
			return;
		}
		else
		{
		_encode_as_object:
			out = soup::make_unique<soup::JsonObject>();
			auto& obj = out->reinterpretAsObj();
			lua_pushvalue(L, i);
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
						luaE_incCstack(L);
						auto& entry = obj.children.emplace_back();
						checkJson(L, -2, entry.first);
						checkJson(L, -1, entry.second);
						L->nCcalls--;
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
					luaE_incCstack(L);
					auto& entry = obj.children.emplace_back();
					checkJson(L, -1, entry.first);
					checkJson(L, -2, entry.second);
					L->nCcalls--;
					lua_pop(L, 2);
				}
			}
			lua_pop(L, 1);
			return;
		}
	}
	else if (type == LUA_TLIGHTUSERDATA)
	{
		if (reinterpret_cast<uintptr_t>(lua_touserdata(L, i)) == 0xF01D)
		{
			out = soup::make_unique<soup::JsonNull>();
			return;
		}
	}
	luaL_typeerror(L, i, "JSON-castable type");
}

static int encode(lua_State* L) {
	int fmt = 0; // compact
	if (lua_type(L, 2) == LUA_TBOOLEAN)
	{
		if (lua_istrue(L, 2))
		{
			fmt = 1; // pretty
		}
	}
	else
	{
		static const char* const fmts[] = { "compact", "pretty", "msgpack", nullptr };
		fmt = luaL_checkoption(L, 2, "compact", fmts);
	}

	auto& up = *pluto_newclassinst(L, soup::UniquePtr<soup::JsonNode>);
	checkJson(L, 1, up);
	
	switch (fmt)
	{
	case 0: default:
		pluto_pushstring(L, up->encode());
		break;

	case 1:
		pluto_pushstring(L, up->encodePretty());
		break;

	case 2:
		soup::StringWriter sw;
		up->msgpackEncode(sw);
		pluto_pushstring(L, std::move(sw.data));
		break;
	}
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
		if (flags & (1 << 2)) // json.msgpack
		{
			soup::MemoryRefReader r(data, size);
			if (soup::json::msgpackDecode(jtw, (void*)L, r, 100))
			{
				return 1;
			}
		}
		else
		{
			if (soup::json::decode(jtw, (void*)L, data, size, 100))
			{
				return 1;
			}
		}
	}
	catch (const std::exception& e)
	{
		luaL_error(L, "%s", e.what());
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
	lua_pushinteger(L, 1 << 2);
	lua_setfield(L, -2, "msgpack");

	return 1;
}
const Pluto::PreloadedLibrary Pluto::preloaded_json{ PLUTO_JSONLIBNAME, funcs, &luaopen_json };
