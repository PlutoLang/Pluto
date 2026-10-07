#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "vendor/Soup/soup/base.hpp"
#if SOUP_CPP20
#include "vendor/Soup/soup/base58.hpp"
#endif

static int encode(lua_State* L) {
#if SOUP_CPP20
	size_t size;
	const char *data = luaL_checklstring(L, 1, &size);
	const bool pad = lua_toboolean(L, 2);
	pluto_pushstring(L, pad ? soup::base58::encodeWithPadding(data, size) : soup::base58::encode(data, size));
	return 1;
#else
	luaL_error(L, "The base58 library requires Pluto to be compiled as C++ 20 or above");
#endif
}

static int decode(lua_State* L) {
#if SOUP_CPP20
	size_t size;
	const char* data = luaL_checklstring(L, 1, &size);
	pluto_pushstring(L, soup::base58::decode(data, size));
	return 1;
#else
	luaL_error(L, "The base58 library requires Pluto to be compiled as C++ 20 or above");
#endif
}

static const luaL_Reg funcs_base58[] = {
	{"encode", encode},
	{"decode", decode},
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base58)
