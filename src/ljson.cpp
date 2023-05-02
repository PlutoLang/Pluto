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

const Pluto::Preloaded Pluto::preloaded_json{ "json", funcs };

LUA_API int luaopen_json(lua_State *L) {
	luaL_newlib(L, funcs);
	return 1;
}

#endif
