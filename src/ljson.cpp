#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "ljson.hpp"

#include "vendor/Soup/soup/string.hpp"

static int encode(lua_State* L) {
	auto& up = *pluto_newclassinst(L, soup::UniquePtr<soup::JsonNode>);
	checkJson(L, 1, up);
	if (lua_istrue(L, 2))
	{
		pluto_pushstring(L, up->encodePretty());
	}
	else
	{
		pluto_pushstring(L, up->encode());
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
	std::string* what = nullptr;
	try
	{
		if (soup::json::decode(jtw, (void*)L, data, size, 100))
		{
			return 1;
		}
	}
	catch (const std::exception& e)
	{
		what = pluto_newclassinst(L, std::string);
		*what = e.what();
	}
	if (what)
	{
		luaL_error(L, what->c_str());
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
const Pluto::PreloadedLibrary Pluto::preloaded_json{ PLUTO_JSONLIBNAME, funcs, &luaopen_json };
