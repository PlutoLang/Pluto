#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_star (lua_State *L) {
  const auto code = R"EOC(local t = {}
for k, v in package.preload do
  if k ~= "*" then
    t[k] = require $"pluto:{k}"
  end
end
return t)EOC";
  luaL_loadbuffer(L, code, strlen(code), "pluto:assert");
  lua_call(L, 0, 1);
  return 1;
}

const Pluto::PreloadedLibrary Pluto::preloaded_star{ "*", funcs, &luaopen_star };
