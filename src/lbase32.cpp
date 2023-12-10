#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "vendor/Soup/soup/base32.hpp"

static int encode(lua_State* L) {
	lua_pushstring(L, soup::base32::encode(luaL_checkstring(L, 1), (bool)lua_toboolean(L, 2)).c_str());
	return 1;
}

static int decode(lua_State* L) {
	lua_pushstring(L, soup::base32::decode(luaL_checkstring(L, 1)).c_str());
	return 1;
}

static const luaL_Reg funcs[] = {
	{"encode", encode},
	{"decode", decode},
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base32)
