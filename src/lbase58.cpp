#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#ifdef PLUTO_USE_SOUP

#include "vendor/Soup/string.hpp"
#include "vendor/Soup/base58.hpp"

static int decode(lua_State* L) {
	try {
		lua_pushstring(L, soup::string::bin2hex(soup::base58::decode(luaL_checkstring(L, 1))).c_str());
	}
	catch (...) {
		luaL_error(L, "Attempted to decode non-base58 string.");
	}
	return 1;
}

static int is_valid(lua_State* L) {
	try {
		const auto discarding = soup::base58::decode(luaL_checkstring(L, 1));
		lua_pushboolean(L, true);
	}
	catch (...) {
		lua_pushboolean(L, false);
	}
	return 1;
}

static const luaL_Reg funcs[] = {
	{"is_valid", is_valid},
	{"decode", decode},
	{nullptr, nullptr}
};

const Pluto::Preloaded Pluto::preloaded_base58{ "base58", funcs };

LUAMOD_API int luaopen_base58(lua_State* L) {
	luaL_newlib(L, funcs);
	return 1;
}

#endif