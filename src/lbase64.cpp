#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"
#include "lstring.h"

#include <string>
#include "vendor/Soup/soup/base64.hpp"

static int encode(lua_State* L) {
	size_t len;
	const char *str = luaL_checklstring(L, 1, &len);
	const bool pad = (lua_gettop(L) >= 2 ? lua_toboolean(L, 2) : true);
	size_t out_len = soup::base64::getEncodedSize(len, pad);
	char shrtbuf[LUAI_MAXSHORTLEN];
	char *enc = plutoS_prealloc(L, shrtbuf, out_len);
	soup::base64::encode(enc, str, len, pad);
	plutoS_commit(L, enc, out_len);
	return 1;
}

static int decode(lua_State* L) {
	size_t len;
	const char* str = luaL_checklstring(L, 1, &len);
	size_t out_len = soup::base64::getDecodedSize(str, len);
	char shrtbuf[LUAI_MAXSHORTLEN];
	char* dec = plutoS_prealloc(L, shrtbuf, out_len);
	soup::base64::decode(dec, str, len);
	plutoS_commit(L, dec, out_len);
	return 1;
}

static int urlEncode(lua_State* L) {
	size_t len;
	const char *str = luaL_checklstring(L, 1, &len);
	const bool pad = lua_toboolean(L, 2);
	size_t out_len = soup::base64::getEncodedSize(len, pad);
	char shrtbuf[LUAI_MAXSHORTLEN];
	char *enc = plutoS_prealloc(L, shrtbuf, out_len);
	soup::base64::urlEncode(enc, str, len, pad);
	plutoS_commit(L, enc, out_len);
	return 1;
}

static int urlDecode(lua_State* L) {
	size_t len;
	const char* str = luaL_checklstring(L, 1, &len);
	size_t out_len = soup::base64::getDecodedSize(str, len);
	char shrtbuf[LUAI_MAXSHORTLEN];
	char* dec = plutoS_prealloc(L, shrtbuf, out_len);
	soup::base64::urlDecode(dec, str, len);
	plutoS_commit(L, dec, out_len);
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
