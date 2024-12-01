#define LUA_LIB

#include <sstream>

#include "lua.h"
#include "lualib.h"
#include "lprefix.h"
#include "luaconf.h"
#include "lauxlib.h"
#include "lstring.h"
#include "lcryptolib.hpp"

#include "vendor/Soup/soup/adler32.hpp"
#include "vendor/Soup/soup/aes.hpp"
#include "vendor/Soup/soup/crc32.hpp"
#include "vendor/Soup/soup/deflate.hpp"
#include "vendor/Soup/soup/HardwareRng.hpp"
#include "vendor/Soup/soup/ripemd160.hpp"
#include "vendor/Soup/soup/rsa.hpp"
#include "vendor/Soup/soup/sha1.hpp"
#include "vendor/Soup/soup/sha256.hpp"
#include "vendor/Soup/soup/sha384.hpp"
#include "vendor/Soup/soup/sha512.hpp"
#include "vendor/Soup/soup/string.hpp"


static int fnv1(lua_State *L)
{
  const auto FNV_offset_basis = 0xcbf29ce484222325ull;
  const auto FNV_prime = 0x100000001b3ull;
  uint64_t hash     = FNV_offset_basis;
  
  size_t l;
  const char* s = luaL_checklstring(L, 1, &l);
  for (; l--; ++s) {
    hash *= FNV_prime;
    hash ^= (uint8_t)*s;
  }

  lua_pushinteger(L, hash);
  return 1;
}


static int fnv1a(lua_State *L)
{
  const auto FNV_offset_basis = 0xcbf29ce484222325ull;
  const auto FNV_prime = 0x100000001b3ull;
  uint64_t hash     = FNV_offset_basis;
  
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
  for (; l--; ++s) {
    hash ^= (uint8_t)*s;
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
  unsigned long hash = 0;
 
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
  for (; l--; ++s) {
    hash = (hash * 33) ^ (unsigned long)*s;
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
  uint32_t hash;

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
  unsigned char buffer[16];
  const auto str = luaL_checklstring(L, 1, &len);
  md5_fn((unsigned char*)str, (int)len, buffer);
  
  char hexbuff[32];
  soup::string::bin2hexAt(hexbuff, (const char*)buffer, sizeof(buffer), soup::string::charset_hex_lower);
  lua_pushlstring(L, hexbuff, sizeof(hexbuff));
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
  const auto hash = soup::crc32::hash((const uint8_t*)text, len, (uint32_t)luaL_optinteger(L, 2, 0));
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


template <typename T>
static int l_hashwithdigest (lua_State *L) {
  size_t l;
  const char *text = luaL_checklstring(L, 1, &l);
  const bool binary = lua_istrue(L, 2);

  typename T::State st;
  st.append(text, l);
  st.finalise();

  char shrtbuf[LUAI_MAXSHORTLEN];
  if (binary) {
    char *out = plutoS_prealloc(L, shrtbuf, T::DIGEST_BYTES);
    st.getDigest(reinterpret_cast<uint8_t*>(out));
    plutoS_commit(L, out, T::DIGEST_BYTES);
  }
  else {
    uint8_t digest[T::DIGEST_BYTES];
    st.getDigest(digest);

    char *out = plutoS_prealloc(L, shrtbuf, T::DIGEST_BYTES * 2);
    soup::string::bin2hexAt(out, (const char*)digest, sizeof(digest), soup::string::charset_hex_lower);
    plutoS_commit(L, out, T::DIGEST_BYTES * 2);
  }
  return 1;
}


static int random(lua_State *L)
{
  lua_pushinteger(L, static_cast<lua_Integer>(soup::FastHardwareRng::generate64()));
  return 1;
}


static int hexdigest(lua_State *L)
{
  pluto_warning(L, "hexdigest(n) is deprecated; use string.format(\"0x%x\", n) instead.");

  std::stringstream stream;
  stream << "0x";
  stream << std::hex << luaL_checkinteger(L, 1);
  lua_pushstring(L, stream.str().c_str());
  return 1;
}


void pushbigint (lua_State *L, soup::Bigint&& x);
soup::Bigint* checkbigint (lua_State *L, int i);


static int generatekeypair (lua_State *L) {
  auto type = luaL_checkstring(L, 1);
  auto bits = (int)luaL_checkinteger(L, 2);
  if (strcmp(type, "rsa") != 0) {
    luaL_error(L, "Unknown type");
  }
  auto kp = soup::RsaKeypair::generate(bits < 0 ? bits * -1 : bits, bits < 0);
  lua_newtable(L);
  lua_pushliteral(L, "n");
  pushbigint(L, std::move(kp.n));
  lua_settable(L, -3);
  lua_pushliteral(L, "e");
  pushbigint(L, std::move(kp.e));
  lua_settable(L, -3);
  lua_newtable(L);
  lua_pushliteral(L, "p");
  pushbigint(L, std::move(kp.p));
  lua_settable(L, -3);
  lua_pushliteral(L, "q");
  pushbigint(L, std::move(kp.q));
  lua_settable(L, -3);
  return 2;
}


static int exportkey (lua_State *L) {
  const char *format = luaL_checkstring(L, 2);
  if (strcmp(format, "pem") == 0) {
    luaL_checktype(L, 1, LUA_TTABLE);
    soup::Bigint *p = lua_getfield(L, 1, "p") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (p) lua_pop(L, 1);
    soup::Bigint *q = lua_getfield(L, 1, "q") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (q) lua_pop(L, 1);
    if (p && q) {
      pluto_pushstring(L, soup::RsaPrivateKey::fromPrimes(*p, *q).toPem());
      return 1;
    }
    else luaL_error(L, "Invalid private key");
  }
  else luaL_error(L, "Unknown format");
}


static int importkey (lua_State *L) {
  const char *format = luaL_checkstring(L, 2);
  if (strcmp(format, "pem") == 0) {
    auto priv = soup::RsaPrivateKey::fromPem(pluto_checkstring(L, 1));
    lua_newtable(L);
    lua_pushliteral(L, "p");
    pushbigint(L, std::move(priv.p));
    lua_settable(L, -3);
    lua_pushliteral(L, "q");
    pushbigint(L, std::move(priv.q));
    lua_settable(L, -3);
    return 1;
  }
  else luaL_error(L, "Unknown format");
}


static int l_encrypt (lua_State *L) {
  size_t mode_len;
  const char *mode = luaL_checklstring(L, 2, &mode_len);
  if (mode_len >= 7 && memcmp(mode, "aes-", 4) == 0) {
    size_t data_len;
    const char *in_data = luaL_checklstring(L, 1, &data_len);
    char *data = reinterpret_cast<char*>(lua_newuserdata(L, data_len + 15 + 16));  /* need up to 15 for alignment and up to 16 for padding */
    if (reinterpret_cast<uintptr_t>(data) % 16) {  /* data is not aligned? */
      data = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(data) + (16 - (reinterpret_cast<uintptr_t>(data) % 16)));  /* align data */
    }
    memcpy(data, in_data, data_len);
    const char *aadata = NULL;  /* for AEAD ciphers */
    size_t aadata_len;
    const char *key;
    size_t key_len;
    const char *iv = NULL;
    size_t iv_len;
    if (memcmp(&mode[4], "cbc", 3) == 0 || memcmp(&mode[4], "cfb", 3) == 0) {
      key = luaL_checklstring(L, 3, &key_len);
      iv = luaL_checklstring(L, 4, &iv_len);
    }
    else if (memcmp(&mode[4], "ecb", 3) == 0) {
      key = luaL_checklstring(L, 3, &key_len);
    }
    else if (memcmp(&mode[4], "gcm", 3) == 0) {
      aadata = luaL_checklstring(L, 3, &aadata_len);
      key = luaL_checklstring(L, 4, &key_len);
      iv = luaL_checklstring(L, 5, &iv_len);
    }
    else {
      luaL_error(L, "Unknown mode");
    }

    if (key_len != 16 && key_len != 24 && key_len != 32) {
      luaL_error(L, "Key length must be 16, 24, or 32 bytes for 128, 192, or 256-bit AES, respectively.");
    }
    if (!aadata && iv && iv_len != 16) {
      luaL_error(L, "IV must be 16 bytes");
    }

    if (mode_len != 7) {
      if (!aadata && mode_len == 13 && memcmp(&mode[7], "-pkcs7", 6) == 0) {
        size_t next_aligned_size = ((data_len / 16) + 1) * 16;
        auto pad_size = static_cast<char>(next_aligned_size - data_len);
        for (size_t i = data_len; i != next_aligned_size; ++i) {
          data[i] = pad_size;
        }
        data_len = next_aligned_size;
      }
      else {
        luaL_error(L, "Unknown mode");
      }
    }

    uint8_t tag[16];
    if (memcmp(&mode[4], "cbc", 3) == 0) {
      soup::aes::cbcEncrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv)
      );
    }
    else if (memcmp(&mode[4], "cfb", 3) == 0) {
      soup::aes::cfbEncrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv)
      );
    }
    else if (memcmp(&mode[4], "ecb", 3) == 0) {
      soup::aes::ecbEncrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len
      );
    }
    else if (memcmp(&mode[4], "gcm", 3) == 0) {
      soup::aes::gcmEncrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(aadata), aadata_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv), iv_len,
        tag
      );
    }

    lua_pushlstring(L, data, data_len);
    if (aadata) {
      lua_remove(L, -2);
      lua_pushlstring(L, reinterpret_cast<const char*>(tag), 16);
      return 2;
    }
    return 1;
  }
  else if (mode_len >= 3 && memcmp(mode, "rsa", 3) == 0) {
    luaL_checktype(L, 3, LUA_TTABLE);
    soup::Bigint *n = lua_getfield(L, 3, "n") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (n) lua_pop(L, 1);
    soup::Bigint *e = lua_getfield(L, 3, "e") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (e) lua_pop(L, 1);
    soup::Bigint *p = lua_getfield(L, 3, "p") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (p) lua_pop(L, 1);
    soup::Bigint *q = lua_getfield(L, 3, "q") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (q) lua_pop(L, 1);
    if (p && q) {  /* private key? */
      std::string data = pluto_checkstring(L, 1);
      try {
        if (strcmp(mode, "rsa-pkcs1") == 0) {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).encryptPkcs1(std::move(data)).toBinary();
        }
        else {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).encryptUnpadded(std::move(data)).toBinary();
        }
      }
      catch (std::exception&) {  /* Either "Assertion failed" or "Modular multiplicative inverse does not exist as the numbers are not coprime" */
        data.clear(); data.shrink_to_fit();
        luaL_error(L, "Invalid private key");
      }
      pluto_pushstring(L, data);
      return 1;
    }
    else if (n && e) {  /* public key? */
      std::string data = pluto_checkstring(L, 1);
      if (strcmp(mode, "rsa-pkcs1") == 0) {
        data = soup::RsaPublicKey(*n, *e).encryptPkcs1(std::move(data)).toBinary();
      }
      else {
        data = soup::RsaPublicKey(*n, *e).encryptUnpadded(std::move(data)).toBinary();
      }
      pluto_pushstring(L, data);
      return 1;
    }
    else luaL_error(L, "Invalid key");
  }
  else luaL_error(L, "Unknown mode");
}


static int l_decrypt (lua_State *L) {
  size_t mode_len;
  const char *mode = luaL_checklstring(L, 2, &mode_len);
  if (mode_len >= 7 && memcmp(mode, "aes-", 4) == 0) {
    size_t data_len;
    const char *in_data = luaL_checklstring(L, 1, &data_len);
    char *data = reinterpret_cast<char*>(lua_newuserdata(L, data_len + 15));  /* need up to 15 for alignment */
    if (reinterpret_cast<uintptr_t>(data) % 16) {  /* data is not aligned? */
      data = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(data) + (16 - (reinterpret_cast<uintptr_t>(data) % 16)));  /* align data */
    }
    memcpy(data, in_data, data_len);
    const char *aadata = NULL;  /* for AEAD ciphers */
    size_t aadata_len;
    const char *key;
    size_t key_len;
    const char *iv = NULL;
    size_t iv_len;
    const char *tag;  /* for AEAD ciphers */
    if (memcmp(&mode[4], "cbc", 3) == 0 || memcmp(&mode[4], "cfb", 3) == 0) {
      key = luaL_checklstring(L, 3, &key_len);
      iv = luaL_checklstring(L, 4, &iv_len);
    }
    else if (memcmp(&mode[4], "ecb", 3) == 0) {
      key = luaL_checklstring(L, 3, &key_len);
    }
    else if (memcmp(&mode[4], "gcm", 3) == 0) {
      aadata = luaL_checklstring(L, 3, &aadata_len);
      key = luaL_checklstring(L, 4, &key_len);
      iv = luaL_checklstring(L, 5, &iv_len);
      size_t tag_len;
      tag = luaL_checklstring(L, 6, &tag_len);
      if (tag_len != 16) {
        luaL_error(L, "Authentication Tag must be 16 bytes");
      }
    }
    else {
      luaL_error(L, "Unknown mode");
    }

    if (key_len != 16 && key_len != 24 && key_len != 32) {
      luaL_error(L, "Key length must be 16, 24, or 32 bytes for 128, 192, or 256-bit AES, respectively.");
    }
    if (!aadata && iv && iv_len != 16) {
      luaL_error(L, "IV must be 16 bytes");
    }

    bool pkcs7 = false;
    if (mode_len != 7) {
      if (!aadata && mode_len == 13 && memcmp(&mode[7], "-pkcs7", 6) == 0) {
        pkcs7 = true;
      }
      else {
        luaL_error(L, "Unknown mode");
      }
    }

    if (memcmp(&mode[4], "cbc", 3) == 0) {
      soup::aes::cbcDecrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv)
      );
    }
    else if (memcmp(&mode[4], "cfb", 3) == 0) {
      soup::aes::cfbDecrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv)
      );
    }
    else if (memcmp(&mode[4], "ecb", 3) == 0) {
      soup::aes::ecbDecrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(key), key_len
      );
    }
    else if (memcmp(&mode[4], "gcm", 3) == 0) {
      if (!soup::aes::gcmDecrypt(
        reinterpret_cast<uint8_t*>(data), data_len,
        reinterpret_cast<const uint8_t*>(aadata), aadata_len,
        reinterpret_cast<const uint8_t*>(key), key_len,
        reinterpret_cast<const uint8_t*>(iv), iv_len,
        reinterpret_cast<const uint8_t*>(tag)
      )) {
        luaL_error(L, "AES-GCM authentication failed");
      }
    }

    if (pkcs7) {
      char pad_size = data[data_len - 1];
      if (l_unlikely(pad_size < 1 || pad_size > 16)) {
        luaL_error(L, "PKCS#7 unpadding failed");
      }
      for (auto i = pad_size; i; --i) {
        if (l_unlikely(data[--data_len] != pad_size)) {
          luaL_error(L, "PKCS#7 unpadding failed");
        }
      }
    }

    lua_pushlstring(L, data, data_len);
    return 1;
  }
  else if (mode_len >= 3 && memcmp(mode, "rsa", 3) == 0) {
    luaL_checktype(L, 3, LUA_TTABLE);
    soup::Bigint *n = lua_getfield(L, 3, "n") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (n) lua_pop(L, 1);
    soup::Bigint *e = lua_getfield(L, 3, "e") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (e) lua_pop(L, 1);
    soup::Bigint *p = lua_getfield(L, 3, "p") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (p) lua_pop(L, 1);
    soup::Bigint *q = lua_getfield(L, 3, "q") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (q) lua_pop(L, 1);
    if (p && q) {  /* private key? */
      std::string data = pluto_checkstring(L, 1);
      try {
        if (strcmp(mode, "rsa-pkcs1") == 0) {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).decryptPkcs1(soup::Bigint::fromBinary(data));
        }
        else {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).decryptUnpadded(soup::Bigint::fromBinary(data));
        }
      }
      catch (std::exception&) {  /* Either "Assertion failed" or "Modular multiplicative inverse does not exist as the numbers are not coprime" */
        data.clear(); data.shrink_to_fit();
        luaL_error(L, "Invalid private key");
      }
      pluto_pushstring(L, data);
      return 1;
    }
    else if (n && e) {  /* public key? */
      std::string data = pluto_checkstring(L, 1);
      if (strcmp(mode, "rsa-pkcs1") == 0) {
        data = soup::RsaPublicKey(*n, *e).decryptPkcs1(soup::Bigint::fromBinary(data));
      }
      else {
        data = soup::RsaPublicKey(*n, *e).decryptUnpadded(soup::Bigint::fromBinary(data));
      }
      pluto_pushstring(L, data);
      return 1;
    }
    else luaL_error(L, "Invalid key");
  }
  else luaL_error(L, "Unknown mode");
}


static int l_sign (lua_State *L) {
  const char *mode = luaL_checkstring(L, 2);
  if (strcmp(mode, "rsa-sha1") == 0 || strcmp(mode, "rsa-sha256") == 0) {
    luaL_checktype(L, 3, LUA_TTABLE);
    soup::Bigint *p = lua_getfield(L, 3, "p") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (p) lua_pop(L, 1);
    soup::Bigint *q = lua_getfield(L, 3, "q") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (q) lua_pop(L, 1);
    if (p && q) {
      std::string data = pluto_checkstring(L, 1);
      try {
        if (strcmp(mode, "rsa-sha1") == 0) {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).sign<soup::sha1>(data).toBinary();
        }
        else {
          data = soup::RsaPrivateKey::fromPrimes(*p, *q).sign<soup::sha256>(data).toBinary();
        }
      }
      catch (std::exception&) {  /* Either "Assertion failed" or "Modular multiplicative inverse does not exist as the numbers are not coprime" */
        data.clear(); data.shrink_to_fit();
        luaL_error(L, "Invalid private key");
      }
      pluto_pushstring(L, data);
      return 1;
    }
    else luaL_error(L, "Invalid private key");
  }
  else luaL_error(L, "Unknown mode");
}


static int l_verify (lua_State *L) {
  const char *mode = luaL_checkstring(L, 2);
  if (strcmp(mode, "rsa-sha1") == 0 || strcmp(mode, "rsa-sha256") == 0) {
    luaL_checktype(L, 3, LUA_TTABLE);
    soup::Bigint *n = lua_getfield(L, 3, "n") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (n) lua_pop(L, 1);
    soup::Bigint *e = lua_getfield(L, 3, "e") == LUA_TUSERDATA ? checkbigint(L, -1) : nullptr; if (e) lua_pop(L, 1);
    if (n && e) {
      std::string data = pluto_checkstring(L, 1);
      std::string sig = pluto_checkstring(L, 4);
      if (strcmp(mode, "rsa-sha1") == 0) {
        lua_pushboolean(L, soup::RsaPublicKey(*n, *e).verify<soup::sha1>(data, soup::Bigint::fromBinary(sig)));
      }
      else {
        lua_pushboolean(L, soup::RsaPublicKey(*n, *e).verify<soup::sha256>(data, soup::Bigint::fromBinary(sig)));
      }
      return 1;
    }
    else luaL_error(L, "Invalid public key");
  }
  else luaL_error(L, "Unknown mode");
}


static int l_adler32 (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  lua_pushinteger(L, soup::adler32::hash(data, size));
  return 1;
}


static int l_decompress (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  soup::deflate::DecompressResult res;
  if (lua_gettop(L) >= 2)
    res = soup::deflate::decompress(data, size, luaL_checkinteger(L, 2));
  else
    res = soup::deflate::decompress(data, size);
  pluto_pushstring(L, res.decompressed);
  lua_newtable(L);
  lua_pushliteral(L, "compressed_size");
  lua_pushinteger(L, res.compressed_size);
  lua_settable(L, -3);
  lua_pushliteral(L, "checksum_present");
  lua_pushboolean(L, res.checksum_present);
  lua_settable(L, -3);
  lua_pushliteral(L, "checksum_mismatch");
  lua_pushboolean(L, res.checksum_mismatch);
  lua_settable(L, -3);
  return 2;
}


static int l_ripemd160 (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  const bool binary = lua_istrue(L, 2);
  auto digest = soup::ripemd160(data, size);
  if (!binary) {
    digest = soup::string::bin2hexLower(digest);
  }
  pluto_pushstring(L, digest);
  return 1;
}


static const luaL_Reg funcs_crypto[] = {
  {"hexdigest", hexdigest},  /* deprecated since 0.8.0 */
  {"random", random},
  {"sha1", l_hashwithdigest<soup::sha1>},
  {"sha256", l_hashwithdigest<soup::sha256>},
  {"sha384", l_hashwithdigest<soup::sha384>},
  {"sha512", l_hashwithdigest<soup::sha512>},
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
  {"generatekeypair", generatekeypair},
  {"exportkey", exportkey},
  {"importkey", importkey},
  {"encrypt", l_encrypt},
  {"decrypt", l_decrypt},
  {"sign", l_sign},
  {"verify", l_verify},
  {"adler32", l_adler32},
  {"decompress", l_decompress},
  {"ripemd160", l_ripemd160},
  {NULL, NULL}
};

PLUTO_NEWLIB(crypto)

