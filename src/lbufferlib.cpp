#define LUA_LIB
#include "lualib.h"

#include <memory> // destroy_at

#include "vendor/Soup/soup/Buffer.hpp"

static soup::Buffer* checkbuffer (lua_State *L, int i) {
  return (soup::Buffer*)luaL_checkudata(L, i, "pluto:buffer");
}

static int buffer_new (lua_State *L) {
  new (lua_newuserdata(L, sizeof(soup::Buffer))) soup::Buffer{};
  if (luaL_newmetatable(L, "pluto:buffer")) {
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:buffer\"", 28, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkbuffer(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  return 1;
}

static int buffer_append (lua_State *L) {
  size_t size;
  const char* data = luaL_checklstring(L, 2, &size);
  checkbuffer(L, 1)->append(data, size);
  return 0;
}

static int buffer_tostring (lua_State *L) {
  const auto buf = checkbuffer(L, 1);
  lua_pushlstring(L, (const char*)buf->data(), buf->size());
  return 1;
}

static const luaL_Reg funcs_buffer[] = {
  {"new", buffer_new},
  {"append", buffer_append},
  {"tostring", buffer_tostring},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(buffer)
