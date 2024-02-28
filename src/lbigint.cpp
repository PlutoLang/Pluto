#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/Bigint.hpp"

void pushbigint (lua_State *L, soup::Bigint&& x);

soup::Bigint* checkbigint (lua_State *L, int i) {
  return (soup::Bigint*)luaL_checkudata(L, i, "pluto:bigint");
}

static int bigint_new (lua_State *L) {
  size_t len;
  const char *str = lua_tolstring(L, 1, &len);
  pushbigint(L, soup::Bigint::fromString(str, len));
  return 1;
}

static int bigint_add (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) + *checkbigint(L, 2));
  return 1;
}

static int bigint_sub (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) - *checkbigint(L, 2));
  return 1;
}

static int bigint_mul (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) * *checkbigint(L, 2));
  return 1;
}

static int bigint_div (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) / *checkbigint(L, 2));
  return 1;
}

static int bigint_mod (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) % *checkbigint(L, 2));
  return 1;
}

static int bigint_exp (lua_State *L) {
  pushbigint(L, checkbigint(L, 1)->pow(*checkbigint(L, 2)));
  return 1;
}

static int bigint_tostring (lua_State *L) {
  pluto_pushstring(L, checkbigint(L, 1)->toString());
  return 1;
}

void pushbigint (lua_State *L, soup::Bigint&& x) {
  if (l_unlikely(luaL_newmetatable(L, "pluto:bigint"))) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State* L) {
      std::destroy_at<>(checkbigint(L, 1));
      return 0;
    });
    lua_settable(L, -3);
    lua_pushliteral(L, "__add");
    lua_pushcfunction(L, bigint_add);
    lua_settable(L, -3);
    lua_pushliteral(L, "__sub");
    lua_pushcfunction(L, bigint_sub);
    lua_settable(L, -3);
    lua_pushliteral(L, "__mul");
    lua_pushcfunction(L, bigint_mul);
    lua_settable(L, -3);
    lua_pushliteral(L, "__div");
    lua_pushcfunction(L, bigint_div);
    lua_settable(L, -3);
    lua_pushliteral(L, "__mod");
    lua_pushcfunction(L, bigint_mod);
    lua_settable(L, -3);
    lua_pushliteral(L, "__pow");
    lua_pushcfunction(L, bigint_exp);
    lua_settable(L, -3);
    lua_pushliteral(L, "__tostring");
    lua_pushcfunction(L, bigint_tostring);
    lua_settable(L, -3);
  }
  lua_pop(L, 1);

  new (lua_newuserdata(L, sizeof(soup::Bigint))) soup::Bigint (std::move(x));
  luaL_setmetatable(L, "pluto:bigint");
}

static const luaL_Reg funcs[] = {
  {"new", bigint_new},
  {"add", bigint_add},
  {"sub", bigint_sub},
  {"mul", bigint_mul},
  {"div", bigint_div},
  {"mod", bigint_mod},
  {"tostring", bigint_tostring},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(bigint);
