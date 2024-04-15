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
  auto [q, r] = checkbigint(L, 1)->divide(*checkbigint(L, 2));
  pushbigint(L, std::move(q));
  pushbigint(L, std::move(r));
  return 2;
}

static int bigint_div_mm (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) / *checkbigint(L, 2));
  return 1;
}

static int bigint_mod (lua_State *L) {
  pushbigint(L, *checkbigint(L, 1) % *checkbigint(L, 2));
  return 1;
}

static int bigint_pow (lua_State *L) {
  pushbigint(L, checkbigint(L, 1)->pow(*checkbigint(L, 2)));
  return 1;
}

static int bigint_tostring (lua_State *L) {
  pluto_pushstring(L, checkbigint(L, 1)->toString());
  return 1;
}

static int bigint_eq (lua_State *L) {
  lua_pushboolean(L, *checkbigint(L, 1) == *checkbigint(L, 2));
  return 1;
}

static int bigint_lt (lua_State *L) {
  lua_pushboolean(L, *checkbigint(L, 1) < *checkbigint(L, 2));
  return 1;
}

static int bigint_le (lua_State *L) {
  lua_pushboolean(L, *checkbigint(L, 1) <= *checkbigint(L, 2));
  return 1;
}

static int bigint_hex (lua_State *L) {
  pluto_pushstring(L, checkbigint(L, 1)->toStringHex());
  return 1;
}

static int bigint_binary (lua_State *L) {
  pluto_pushstring(L, checkbigint(L, 1)->toStringBinary());
  return 1;
}

static int bigint_bitlength (lua_State *L) {
  lua_pushinteger(L, checkbigint(L, 1)->getBitLength());
  return 1;
}

void pushbigint (lua_State *L, soup::Bigint&& x) {
  new (lua_newuserdata(L, sizeof(soup::Bigint))) soup::Bigint(std::move(x));
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
    lua_pushcfunction(L, bigint_div_mm);
    lua_settable(L, -3);
    lua_pushliteral(L, "__mod");
    lua_pushcfunction(L, bigint_mod);
    lua_settable(L, -3);
    lua_pushliteral(L, "__pow");
    lua_pushcfunction(L, bigint_pow);
    lua_settable(L, -3);
    lua_pushliteral(L, "__tostring");
    lua_pushcfunction(L, bigint_tostring);
    lua_settable(L, -3);
    lua_pushliteral(L, "__eq");
    lua_pushcfunction(L, bigint_eq);
    lua_settable(L, -3);
    lua_pushliteral(L, "__lt");
    lua_pushcfunction(L, bigint_lt);
    lua_settable(L, -3);
    lua_pushliteral(L, "__le");
    lua_pushcfunction(L, bigint_le);
    lua_settable(L, -3);
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:bigint\"", 28, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
}

static const luaL_Reg funcs_bigint[] = {
  {"new", bigint_new},
  {"add", bigint_add},
  {"sub", bigint_sub},
  {"mul", bigint_mul},
  {"div", bigint_div},
  {"mod", bigint_mod},
  {"pow", bigint_pow},
  {"tostring", bigint_tostring},
  {"eq", bigint_eq},
  {"lt", bigint_lt},
  {"le", bigint_le},
  {"hex", bigint_hex},
  {"binary", bigint_binary},
  {"bitlength", bigint_bitlength},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(bigint);
