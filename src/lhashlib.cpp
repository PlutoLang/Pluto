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
  uint32_t hash = 0;
  const std::string s = luaL_checkstring(L, 1);

	for (auto c : s)
	{
		hash += c;
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

  lua_pushinteger(L, hash);
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
  const std::string input = luaL_checkstring(L, 1);
  const auto seed = luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash1Aligned(input.c_str(), input.size(), seed);
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

