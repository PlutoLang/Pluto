#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "ljson.hpp"

static int encode(lua_State* L)
{
	auto up = new (lua_newuserdata(L, sizeof(soup::UniquePtr<soup::JsonNode>))) soup::UniquePtr<soup::JsonNode>{};
	lua_newtable(L);
	{
		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, [](lua_State* L) -> int
		{
			std::destroy_at((soup::UniquePtr<soup::JsonNode>*)lua_touserdata(L, 1));
			return 0;
		});
		lua_settable(L, -3);
	}
	lua_setmetatable(L, -2);

	checkJson(L, 1, *up);

	if (lua_istrue(L, 2))
	{
		pluto_pushstring(L, (*up)->encodePretty());
	}
	else
	{
		pluto_pushstring(L, (*up)->encode());
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
	jtw.allocArray = [](void* L) -> void* {
		lua_checkstack((lua_State*)L, 3);
		lua_newtable((lua_State*)L);
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
		jtw.allocObject = [](void* L) -> void* {
			lua_checkstack((lua_State*)L, 7);
			lua_newtable((lua_State*)L);
			// stack now: table
			lua_newtable((lua_State*)L);
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
		jtw.allocObject = [](void* L) -> void* {
			lua_checkstack((lua_State*)L, 3);
			lua_newtable((lua_State*)L);
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
