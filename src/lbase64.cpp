#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include <string>
#include "vendor/Soup/soup/base64.hpp"

static int encode(lua_State* L) {
	size_t len;
	auto str = luaL_checklstring(L, 1, &len);
	pluto_pushstring(L, soup::base64::encode(str, len, (bool)(lua_gettop(L) >= 2 ? lua_toboolean(L, 2) : true)));
	return 1;
}

static int decode(lua_State* L) {
	pluto_pushstring(L, soup::base64::decode(pluto_checkstring(L, 1)));
	return 1;
}

static int urlEncode(lua_State* L) {
	size_t len;
	auto str = luaL_checklstring(L, 1, &len);
	pluto_pushstring(L, soup::base64::urlEncode(str, len, (bool)lua_toboolean(L, 2)));
	return 1;
}

static int urlDecode(lua_State* L) {
	pluto_pushstring(L, soup::base64::urlDecode(pluto_checkstring(L, 1)));
	return 1;
}

static int urlEncodeDeprecated(lua_State* L) {
	pluto_warning(L, "url_encode is deprecated, replace the call with urlencode.");
	return urlEncode(L);
}

static int urlDecodeDeprecated(lua_State* L) {
	pluto_warning(L, "url_decode is deprecated, replace the call with urldecode.");
	return urlDecode(L);
}

static const luaL_Reg funcs_base64[] = {
	{"encode", encode},
	{"decode", decode},
	{"urlencode", urlEncode},  /* added in Pluto 0.8.0 */
	{"urldecode", urlDecode},  /* added in Pluto 0.8.0 */
	{"url_encode", urlEncodeDeprecated},  /* deprecated as of Pluto 0.8.0 */
	{"url_decode", urlDecodeDeprecated},  /* deprecated as of Pluto 0.8.0 */
	{nullptr, nullptr}
};

PLUTO_NEWLIB(base64)
