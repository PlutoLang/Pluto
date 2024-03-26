#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/Regex.hpp"

static soup::Regex* checkregex (lua_State *L, int i) {
  return (soup::Regex*)luaL_checkudata(L, i, "pluto:regex");
}

static int regex_new (lua_State *L) {
  new (lua_newuserdata(L, sizeof(soup::Regex))) soup::Regex{ soup::Regex::fromFullString(pluto_checkstring(L, 1)) };
  if (luaL_newmetatable(L, "pluto:regex")) {
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:regex\"", 27, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkregex(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  return 1;
}

static int regex_match (lua_State *L) {
  size_t len;
  const char *str = luaL_checklstring(L, 2, &len);
  auto res = checkregex(L, 1)->match(str, str + len);
  if (res.isSuccess()) {
    lua_newtable(L);
    for (size_t i = 0; i != res.groups.size(); ++i) {
      if (res.groups[i].has_value()) {
        if (res.groups[i]->name.empty())
          lua_pushinteger(L, i);
        else
          pluto_pushstring(L, res.groups[i]->name);
        lua_pushlstring(L, res.groups[i]->begin, res.groups[i]->length());
        lua_settable(L, -3);
      }
    }
  }
  else {
    luaL_pushfail(L);
  }
  return 1;
}

static const luaL_Reg funcs_regex[] = {
  {"new", regex_new},
  {"match", regex_match},
  {nullptr, nullptr}
};
PLUTO_NEWLIB(regex);
