#define LUA_LIB
#include "lualib.h"
#include "llimits.h"

#include "vendor/Soup/soup/Regex.hpp"

static soup::Regex* checkregex (lua_State *L, int i) {
  return (soup::Regex*)luaL_checkudata(L, i, "pluto:regex");
}

static int regex_new (lua_State *L) {
  void *addr = lua_newuserdata(L, sizeof(soup::Regex));
  try {
    new (addr) soup::Regex{ soup::Regex::fromFullString(pluto_checkstring(L, 1)) };
  }
  catch (std::exception& e) {
    luaL_error(L, "%s", e.what());
  }
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

/* This function is identical to JavaScript's String.prototype.match. */
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

size_t posrelatI (lua_Integer pos, size_t len);

/* This function is similar to JavaScript's String.prototype.search. The optional offset parameter can not be found in JavaScript. */
static int regex_search (lua_State *L) {
  size_t len;
  const char *str = luaL_checklstring(L, 2, &len);
  const auto offset = posrelatI(luaL_optinteger(L, 3, 1), len) - 1;
  if (l_unlikely(offset > len)) {
    luaL_error(L, "offset out of range");
  }
  auto res = checkregex(L, 1)->search(str + offset, str + len);
  if (res.isSuccess()) {
    lua_pushinteger(L, 1 + (res.groups.at(0).value().begin - str));
    lua_pushinteger(L, 1 + (res.groups.at(0).value().end - str));
    return 2;
  }
  luaL_pushfail(L);
  return 1;
}

static const luaL_Reg funcs_regex[] = {
  {"new", regex_new},
  {"match", regex_match},
  {"search", regex_search},
  {nullptr, nullptr}
};
PLUTO_NEWLIB(regex);
