#define lhashlib_c
#define LUA_LIB

#include "lua.h"
#include "lualib.h"
#include "lprefix.h"
#include "luaconf.h"
#include "lauxlib.h"
#include "lhashlib.hpp"
#include <string>


static int fnv1(lua_State *L)
{
  const auto FNV_offset_basis = 0xcbf29ce484222325;
  const auto FNV_prime = 0x100000001b3;
  const std::string s  = luaL_checkstring(L, 1);
  lua_Integer hash     = FNV_offset_basis;
  
  for (auto& c : s)
  {
    hash *= FNV_prime;
    hash ^= c;
  }

  lua_pushinteger(L, hash);
  return 1;
}


static int fnv1a(lua_State *L)
{
  const auto FNV_offset_basis = 0xcbf29ce484222325;
  const auto FNV_prime = 0x100000001b3;
  const std::string s  = luaL_checkstring(L, 1);
  lua_Integer hash     = FNV_offset_basis;
  
  for (auto& c : s)
  {
    hash ^= c;
    hash *= FNV_prime;
  }

  lua_pushinteger(L, hash);
  return 1;
}


static int joaat(lua_State *L)
{
  /* get input */
  size_t size;
  const char* data = luaL_checklstring(L, 1, &size);
 
  /* do partial on input */
  size_t v3 = 0;
  uint32_t result = 0; /* initial = 0 */
  int v5 = 0;
  for (; v3 < size; result = ((uint32_t)(1025 * (v5 + result)) >> 6) ^ (1025 * (v5 + result))) {
    v5 = data[v3++];
  }

  /* finalise */
  result = (0x8001 * (((uint32_t)(9 * result) >> 11) ^ (9 * result)));

  /* done, give result */
  lua_pushinteger(L, result);
  return 1;
}


// Times33, a.k.a DJBX33A is the hashing algorithm used in PHP's hash table.
static int times33(lua_State *L)
{
  const std::string str = luaL_checkstring(L, 1);
  unsigned long hash = 0;
 
  for (auto c : str)
  {
    hash = (hash * 33) ^ (unsigned long)c;
  }
  
  lua_pushinteger(L, hash);
  return 1;
}


static int murmur1(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash1Aligned(text, textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}


static const luaL_Reg funcs[] = {
  {"murmur1", murmur1},
  {"times33", times33},
  {"joaat", joaat},
  {"fnv1a", fnv1a},
  {"fnv1", fnv1},
  {NULL, NULL}  
};


LUAMOD_API int luaopen_hash(lua_State *L)
{
  luaL_newlib(L, funcs);
  return 1;
}

