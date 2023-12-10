#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include <string>
#include "vendor/Soup/soup/base64.hpp"

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

static int urlEncodeDeprecated(lua_State* L) {
	lua_warning(L, "base64: url_encode is deprecated, replace the call with urlencode.", 0);
	return urlEncode(L);
}

static int urlDecodeDeprecated(lua_State* L) {
	lua_warning(L, "base64: url_decode is deprecated, replace the call with urldecode.", 0);
	return urlDecode(L);
}

static const luaL_Reg funcs[] = {
	{"encode", encode},
	{"decode", decode},
	{"urlencode", urlEncode},  /* added in Pluto 0.8.0 */
	{"urldecode", urlDecode},  /* added in Pluto 0.8.0 */
	{"url_encode", urlEncodeDeprecated},  /* deprecated as of Pluto 0.8.0 */
	{"url_decode", urlDecodeDeprecated},  /* deprecated as of Pluto 0.8.0 */
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base64)
