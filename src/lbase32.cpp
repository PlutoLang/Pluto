#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#ifdef PLUTO_USE_SOUP

#include "vendor/Soup/base32.hpp"

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

const Pluto::Preloaded Pluto::preloaded_base32{ "base32", funcs };

LUAMOD_API int luaopen_base32(lua_State* L) {
	luaL_newlib(L, funcs);
	return 1;
}

#endif