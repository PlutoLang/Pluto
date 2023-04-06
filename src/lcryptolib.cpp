#define LUA_LIB

#include "lua.h"
#include "lualib.h"
#include "lprefix.h"
#include "luaconf.h"
#include "lauxlib.h"
#include "lstring.h"
#include "lcryptolib.hpp"

#include <ios>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>


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
  const auto seed = (unsigned int)luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash1Aligned(text, (int)textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}

// P1: String
// P2: Seed
// P3: Boolean whether to follow alignment.
static int murmur2(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = luaL_optinteger(L, 2, 0);
  unsigned int hash;

  if (lua_toboolean(L, 3))
    hash = MurmurHashAligned2(text, (int)textLen, (uint32_t)seed);
  else
    hash = MurmurHash2(text, (int)textLen, (uint32_t)seed);

  lua_pushinteger(L, hash);
  return 1;
}


// x86_64
static int murmur64a(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = (uint64_t)luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash64A(text, (int)textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}


// 64-bit hash for 32-bit platforms
static int murmur64b(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = (uint64_t)luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash64B(text, (int)textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}


static int murmur2a(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = (uint32_t)luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHash2A(text, (int)textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}


// Architecture independent.
static int murmur2neutral(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto seed = (uint32_t)luaL_optinteger(L, 2, 0);
  const auto hash = MurmurHashNeutral2(text, (int)textLen, seed);
  lua_pushinteger(L, hash);
  return 1;
}


// Just the name. I still prefer DJB.
static int superfasthash(lua_State *L)
{
  size_t textLen;
  const auto text = luaL_checklstring(L, 1, &textLen);
  const auto hash = SuperFastHash((const signed char*)text, (int)textLen);
  lua_pushinteger(L, hash);
  return 1;
}


static int djb2(lua_State *L)
{
  int c;
  auto str = luaL_checkstring(L, 1);
  unsigned long hash = 5381;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  lua_pushinteger(L, hash);
  return 1;
}


static int sdbm(lua_State *L)
{
  int c;
  auto str = luaL_checkstring(L, 1);
  unsigned long hash = 0;

  while ((c = *str++))
    hash = c + (hash << 6) + (hash << 16) - hash;

  lua_pushinteger(L, hash);
  return 1;
}


static int md5(lua_State *L)
{
  size_t len;
  unsigned char buffer[16] = {};
  const auto str = luaL_checklstring(L, 1, &len);
  md5_fn((unsigned char*)str, (int)len, buffer);
  
  std::stringstream res {};
  for (int i = 0; i < 16; i++)
  {
    res << std::hex << (int)buffer[i];
  }

  lua_pushstring(L, res.str().c_str());
  return 1;
}


static int lookup3(lua_State *L)
{
  size_t len;
  const auto text = luaL_checklstring(L, 1, &len);
  const auto hash = lookup3_impl(text, (int)len, (uint32_t)luaL_optinteger(L, 2, 0));
  lua_pushinteger(L, hash);
  return 1;
}


static int crc32(lua_State *L)
{
  size_t len;
  const auto text = luaL_checklstring(L, 1, &len);
  const auto hash = crc32_impl(text, (int)len, (uint32_t)luaL_optinteger(L, 2, 0));
  lua_pushinteger(L, hash);
  return 1;
}


// The hashing function used inside Lua. Honorary addition.
// Basically a slightly different DJB2.
static int lua(lua_State *L)
{
  size_t l;
  const auto text = luaL_checklstring(L, 1, &l);
  const auto hash = luaS_hash(text, l, (unsigned int)luaL_optinteger(L, 2, 0));
  lua_pushinteger(L, hash);
  return 1;
}


static int l_sha256(lua_State *L)
{
  size_t l;
  char hex[SHA256_HEX_SIZE];
  const auto text = luaL_checklstring(L, 1, &l);
  sha256_hex(text, l, hex);
  lua_pushstring(L, hex);
  return 1;
}


// This should be fairly secure on systems that employ a randomized device, like /dev/urandom, BCryptGenRandom, etc.
// But, otherwise, it's not secure whatsoever.
static int random(lua_State *L)
{
  std::random_device dev;
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__) || defined(_M_X64) || defined(__aarch64__)
  std::mt19937_64 rng(dev());
  std::uniform_int_distribution<std::mt19937_64::result_type> dist(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
#else
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist((std::mt19937::result_type)luaL_checkinteger(L, 1), (std::mt19937::result_type)luaL_checkinteger(L, 2));
#endif
  lua_pushinteger(L, dist(rng));
  return 1;
}


static int hexdigest(lua_State *L)
{
  std::stringstream stream;
  stream << "0x";
  stream << std::hex << luaL_checkinteger(L, 1);
  lua_pushstring(L, stream.str().c_str());
  return 1;
}


static const luaL_Reg funcs[] = {
  {"hexdigest", hexdigest},
  {"random", random},
  {"sha256", l_sha256},
  {"lua", lua},
  {"crc32", crc32},
  {"lookup3", lookup3},
  {"md5", md5},
  {"sdbm", sdbm},
  {"djb2", djb2},
  {"superfasthash", superfasthash},
  {"murmur2neutral", murmur2neutral},
  {"murmur64b", murmur64b},
  {"murmur64a", murmur64a},
  {"murmur2a", murmur2a},
  {"murmur2", murmur2},
  {"murmur1", murmur1},
  {"times33", times33},
  {"joaat", joaat},
  {"fnv1a", fnv1a},
  {"fnv1", fnv1},
  {NULL, NULL}  
};

const Pluto::Preloaded Pluto::preloaded_crypto{ "crypto", funcs };

LUAMOD_API int luaopen_crypto(lua_State *L)
{
  luaL_newlib(L, funcs);
  return 1;
}

