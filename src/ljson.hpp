#pragma once

// This function by itself is not a comprehensive check; the actual array encoding may bail to object if it detects an issue.
inline lua_Integer isIndexBasedTable (lua_State *L, int i) {
  lua_pushvalue(L, i);
  lua_pushnil(L);
  lua_Integer k = 1;
  for (; lua_next(L, -2); ++k) {
    if (lua_type(L, -2) != LUA_TNUMBER) {
      lua_pop(L, 3);
      return 0;
    }
    if (!lua_isinteger(L, -2)) {
      lua_pop(L, 3);
      return 0;
    }
    if (lua_tointeger(L, -2) <= 0) {
      lua_pop(L, 3);
      return 0;
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  return k;
}
