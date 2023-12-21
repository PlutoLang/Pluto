#include <cstring>

#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/cat.hpp"
#include "vendor/Soup/soup/StringRefReader.hpp"

static void cat_encode_aux (lua_State *L, std::string& data, const std::string& prefix = {}) {
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    const char *name = lua_tostring(L, -1);
    lua_pop(L, 1);
    if (strcmp(name, "__value") != 0) {
      data.append(prefix);
      data.append(name);
      if (lua_istable(L, -1)) {
        lua_pushliteral(L, "__value");
        if (lua_rawget(L, -2) > LUA_TNIL) {
          data.append(": ");
          data.append(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        data.push_back('\n');
        cat_encode_aux(L, data, prefix + "\t");
      }
      else {
        data.append(": ");
        data.append(lua_tostring(L, -1));
        data.push_back('\n');
      }
    }
    lua_pop(L, 1);
  }
}

static int cat_encode (lua_State *L) {
  std::string data;
  lua_pushvalue(L, 1);
  cat_encode_aux(L, data);
  lua_pop(L, 1);
  pluto_pushstring(L, data);
  return 1;
}

static void cat_decode_aux (lua_State *L, const soup::catNode& node) {
  if (!node.value.empty()) {
    lua_pushliteral(L, "__value");
    pluto_pushstring(L, node.value);
    lua_settable(L, -3);
  }
  for (const auto& child : node.children) {
    pluto_pushstring(L, child->name);
    if (!child->children.empty()) {
      lua_newtable(L);
      cat_decode_aux(L, *child);
    }
    else {
      pluto_pushstring(L, child->value);
    }
    lua_settable(L, -3);
  }
}

static int cat_decode (lua_State *L) {
  std::string data = pluto_checkstring(L, 1);
  soup::StringRefReader sr(data);
  if (auto root = soup::catParse(sr)) {
    lua_newtable(L);
    cat_decode_aux(L, *root);
    return 1;
  }
  return 0;
}

static const luaL_Reg funcs[] = {
  {"encode", cat_encode},
  {"decode", cat_decode},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(cat)
