#define LUA_LIB
#include "lualib.h"

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

static const luaL_Reg funcs[] = {
  {"encode", url_encode},
  {"decode", url_decode},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(url)
