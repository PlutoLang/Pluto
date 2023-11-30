#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#ifdef PLUTO_USE_SOUP

#include "ljson.hpp"

static int encode(lua_State* L) {
	auto root = checkJson(L, 1);
	if (lua_gettop(L) >= 2 && lua_toboolean(L, 2))
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
	if (auto root = soup::json::decode(luaL_checkstring(L, 1)))
	{
		pushFromJson(L, *root);
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
	lua_pushlightuserdata(L, reinterpret_cast<void*>(static_cast<uintptr_t>('PJNL')));
	lua_setfield(L, -2, "null");
	return 1;
}
const Pluto::PreloadedLibrary Pluto::preloaded_json{ "json", funcs, &luaopen_json };

#endif
