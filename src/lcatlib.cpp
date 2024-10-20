#include <cstring>

#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/cat.hpp"
#include "vendor/Soup/soup/string.hpp"
#include "vendor/Soup/soup/MemoryRefReader.hpp"

static void cat_encode_aux (lua_State *L, std::string& data, const std::string& prefix);

static void cat_encode_value (lua_State *L, std::string& data, const std::string& prefix) {
  if (lua_istable(L, -1)) {
    lua_pushliteral(L, "__value");
    if (lua_rawget(L, -2) > LUA_TNIL) {
      data.append(": ");
      size_t len;
      const char *str = lua_tolstring(L, -1, &len);
      std::string value(str, len);
      soup::cat::encodeValue(value);
      data.append(value);
    }
    lua_pop(L, 1);
    data.push_back('\n');
    cat_encode_aux(L, data, prefix + "\t");
  }
  else {
    data.append(": ");
    size_t len;
    const char* str = lua_tolstring(L, -1, &len);
    std::string value(str, len);
    soup::cat::encodeValue(value);
    data.append(value);
    data.push_back('\n');
  }
}

static void cat_encode_aux (lua_State *L, std::string& data, const std::string& prefix = {}) {
  lua_pushliteral(L, "__order");
  if (lua_rawget(L, -2) == LUA_TTABLE) {
    /* stack: table, order */
    lua_pushnil(L);
    /* stack: table, order, orderidx */
    while (lua_next(L, -2)) {
      /* stack: table, order, orderidx, fieldname */
      lua_pushvalue(L, -1);
      const char *fieldname = lua_tostring(L, -1);
      lua_pop(L, 1);
      if (strcmp(fieldname, "__value") != 0) {
        data.append(prefix);
        data.append(soup::string::replaceAll(fieldname, ":", "\\:"));
        lua_rawget(L, -4);
        /* stack: table, order, orderidx, fieldvalue */
        cat_encode_value(L, data, prefix);
      }
      lua_pop(L, 1);
    }
    return;
  }
  lua_pop(L, 1);

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushvalue(L, -2);
    size_t len;
    const char *str = lua_tolstring(L, -1, &len);
    std::string name(str, len);
    lua_pop(L, 1);
    if (name != "__value") {
      data.append(prefix);
      soup::cat::encodeName(name);
      data.append(name);
      cat_encode_value(L, data, prefix);
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

static void cat_decode_aux_flat (lua_State *L, const soup::catNode& node, bool withorder) {
  if (!node.value.empty()) {
    lua_pushliteral(L, "__value");
    pluto_pushstring(L, node.value);
    lua_settable(L, -3);
  }
  for (const auto& child : node.children) {
    pluto_pushstring(L, child->name);
    if (!child->children.empty()) {
      lua_newtable(L);
      cat_decode_aux_flat(L, *child, withorder);
    }
    else {
      pluto_pushstring(L, child->value);
    }
    lua_settable(L, -3);
  }
  if (withorder) {
    lua_pushliteral(L, "__order");
    lua_newtable(L);
    lua_Integer i = 1;
    for (const auto& child : node.children) {
      lua_pushinteger(L, i++);
      pluto_pushstring(L, child->name);
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
}

static int find_cat_node (lua_State *L) {
  lua_pushnil(L);
  while (lua_next(L, -2)) {
    lua_pushliteral(L, "name");
    lua_rawget(L, -2);
    if (lua_compare(L, 2, -1, LUA_OPEQ)) {
      lua_pop(L, 1);
      return 1;
    }
    lua_pop(L, 2);
  }
  return 0;
}

static void cat_decode_aux_full (lua_State* L, const soup::catNode& node) {
  lua_Integer i = 1;
  for (const auto& child : node.children) {
    lua_pushinteger(L, i++);
    lua_newtable(L);

    lua_pushliteral(L, "name");
    pluto_pushstring(L, child->name);
    lua_settable(L, -3);

    if (!child->value.empty()) {
      lua_pushliteral(L, "value");
      pluto_pushstring(L, child->value);
      lua_settable(L, -3);
    }

    if (!child->children.empty()) {
      lua_pushliteral(L, "children");
      lua_newtable(L);
      cat_decode_aux_full(L, *child);
      lua_settable(L, -3);

      if (luaL_newmetatable(L, "pluto:cat_full_node")) {
        lua_pushliteral(L, "__index");
        lua_pushcfunction(L, [](lua_State *L) -> int {
          lua_pushliteral(L, "children");
          lua_rawget(L, 1);
          return find_cat_node(L);
        });
        lua_settable(L, -3);
      }
      lua_setmetatable(L, -2);
    }

    lua_settable(L, -3);
  }
}

static int cat_decode (lua_State *L) {
  bool flat = false;
  bool withorder = false;
  if (lua_gettop(L) >= 2) {
    const char *mode = luaL_checkstring(L, 2);
    if (strcmp(mode, "flat") == 0)
      flat = true;
    else if (strcmp(mode, "flatwithorder") == 0) {
      flat = true;
      withorder = true;
    }
    else if (strcmp(mode, "full") != 0)
      luaL_error(L, "unknown output format '%s'", mode);
  }
  size_t len;
  const char *data = luaL_checklstring(L, 1, &len);
  soup::MemoryRefReader sr(data, len);
  if (auto root = soup::cat::parse(sr)) {
    lua_newtable(L);
    if (flat)
      cat_decode_aux_flat(L, *root, withorder);
    else {
      cat_decode_aux_full(L, *root);
      lua_newtable(L);
      lua_pushliteral(L, "__index");
      lua_pushcfunction(L, [](lua_State *L) -> int {
        lua_pushvalue(L, 1);
        return find_cat_node(L);
      });
      lua_settable(L, -3);
      lua_setmetatable(L, -2);
    }
    return 1;
  }
  return 0;
}

static const luaL_Reg funcs_cat[] = {
  {"encode", cat_encode},
  {"decode", cat_decode},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(cat)
