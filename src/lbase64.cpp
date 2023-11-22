#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#ifdef PLUTO_USE_SOUP

#include <string>
#include "vendor/Soup/base64.hpp"

static int encode(lua_State* L) {
	lua_pushstring(L, soup::base64::encode(luaL_checkstring(L, 1), (bool)lua_toboolean(L, 2)).c_str());
	return 1;
}

static int decode(lua_State* L) {
	lua_pushstring(L, soup::base64::decode(luaL_checkstring(L, 1)).c_str());
	return 1;
}

static int urlEncode(lua_State* L) {
	lua_pushstring(L, soup::base64::urlEncode(luaL_checkstring(L, 1), (bool)lua_toboolean(L, 2)).c_str());
	return 1;
}

static int urlDecode(lua_State* L) {
	lua_pushstring(L, soup::base64::urlDecode(luaL_checkstring(L, 1)).c_str());
	return 1;
}

static const luaL_Reg funcs[] = {
	{"encode", encode},
	{"decode", decode},
	{"urlencode", urlEncode},  /* added in Pluto 0.8.0 */
	{"urldecode", urlDecode},  /* added in Pluto 0.8.0 */
	{"url_encode", urlEncode},  /* deprecated */
	{"url_decode", urlDecode},  /* deprecated */
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base64)

#endif