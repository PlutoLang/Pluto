#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "vendor/Soup/soup/base32.hpp"

static int encode(lua_State* L) {
	pluto_pushstring(L, soup::base32::encode(pluto_checkstring(L, 1), (bool)(lua_gettop(L) >= 2 ? lua_toboolean(L, 2) : true)));
	return 1;
}

static int decode(lua_State* L) {
	pluto_pushstring(L, soup::base32::decode(pluto_checkstring(L, 1)));
	return 1;
}

static const luaL_Reg funcs_base32[] = {
	{"encode", encode},
	{"decode", decode},
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base32)
