#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "ljson.hpp"

static int encode(lua_State* L) {
	auto root = checkJson(L, 1);
	if (lua_istrue(L, 2))
	{
		pluto_pushstring(L, root->encodePretty());
	}
	else
	{
		pluto_pushstring(L, root->encode());
	}
	return 1;
}

static int decode(lua_State* L)
{
	int flags = 0;
	if (lua_gettop(L) >= 2)
	{
		flags = (int)luaL_checkinteger(L, 2);
	}
	if (auto root = soup::json::decode(pluto_checkstring(L, 1)))
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
