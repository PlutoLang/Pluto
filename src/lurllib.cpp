#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/Uri.hpp"
#include "vendor/Soup/soup/urlenc.hpp"


static int url_encode (lua_State* L) {
  const auto input = pluto_checkstring(L, 1);
  pluto_pushstring(L, soup::urlenc::encode(input));
  return 1;
}


static int url_decode (lua_State *L) {
  const auto input = pluto_checkstring(L, 1);
  pluto_pushstring(L, soup::urlenc::decode(input));
  return 1;
}


static int url_parse (lua_State *L) {
  lua_newtable(L);
  soup::Uri uri(pluto_checkstring(L, 1));
  pluto_pushstring(L, uri.scheme);
  lua_setfield(L, -2, "scheme");
  pluto_pushstring(L, uri.host);
  lua_setfield(L, -2, "host");
  lua_pushinteger(L, uri.port);
  lua_setfield(L, -2, "port");
  pluto_pushstring(L, uri.user);
  lua_setfield(L, -2, "user");
  pluto_pushstring(L, uri.pass);
  lua_setfield(L, -2, "pass");
  pluto_pushstring(L, uri.path);
  lua_setfield(L, -2, "path");
  pluto_pushstring(L, uri.query);
  lua_setfield(L, -2, "query");
  pluto_pushstring(L, uri.fragment);
  lua_setfield(L, -2, "fragment");
  return 1;
}


static const luaL_Reg funcs_url[] = {
  {"encode", url_encode},
  {"decode", url_decode},
  {"parse", url_parse},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(url)
