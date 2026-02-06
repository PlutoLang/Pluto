#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include "vendor/Soup/soup/lzf.hpp"

static int compress (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  auto buffer_size = size + (size >> 5) + 2;
  auto buffer = lua_newuserdata(L, buffer_size);
  if (auto compressed_size = soup::lzf::compress(data, static_cast<unsigned int>(size), buffer, static_cast<unsigned int>(buffer_size))) {
    lua_pushlstring(L, static_cast<char*>(buffer), compressed_size);
    return 1;
  }
  return luaL_error(L, "failed to compress string");
}

static int decompress (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  auto buffer_size = size << 1;
  auto buffer = lua_newuserdata(L, buffer_size);
  if (auto compressed_size = soup::lzf::decompress(data, static_cast<unsigned int>(size), buffer, static_cast<unsigned int>(buffer_size))) {
    lua_pushlstring(L, static_cast<char*>(buffer), compressed_size);
    return 1;
  }
  return luaL_error(L, "failed to decompress string");
}

static const luaL_Reg funcs_lzf[] = {
  {"compress", compress},
  {"decompress", decompress},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(lzf)
