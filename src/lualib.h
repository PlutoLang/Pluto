#pragma once
/*
** $Id: lualib.h $
** Lua standard libraries
** See Copyright Notice in lua.h
*/

#include "lua.h"
#include "lauxlib.h" // Pluto::Preloaded


/* version suffix for environment variable names */
#define LUA_VERSUFFIX          "_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR


LUAMOD_API int (luaopen_base) (lua_State *L);

#define LUA_COLIBNAME	"coroutine"
LUAMOD_API int (luaopen_coroutine) (lua_State *L);

#define LUA_TABLIBNAME	"table"
LUAMOD_API int (luaopen_table) (lua_State *L);

#define LUA_IOLIBNAME	"io"
LUAMOD_API int (luaopen_io) (lua_State *L);

#define LUA_OSLIBNAME	"os"
LUAMOD_API int (luaopen_os) (lua_State *L);

#define LUA_STRLIBNAME	"string"
LUAMOD_API int (luaopen_string) (lua_State *L);

#define LUA_UTF8LIBNAME	"utf8"
LUAMOD_API int (luaopen_utf8) (lua_State *L);

#define LUA_MATHLIBNAME	"math"
LUAMOD_API int (luaopen_math) (lua_State *L);

#define LUA_DBLIBNAME	"debug"
LUAMOD_API int (luaopen_debug) (lua_State *L);

#define LUA_LOADLIBNAME	"package"
LUAMOD_API int (luaopen_package) (lua_State *L);

namespace Pluto {
  extern const PreloadedLibrary preloaded_crypto;
  extern const PreloadedLibrary preloaded_json;
  extern const PreloadedLibrary preloaded_base32;
  extern const PreloadedLibrary preloaded_base64;
  extern const PreloadedLibrary preloaded_assert;
  extern const PreloadedLibrary preloaded_vector3;
  extern const PreloadedLibrary preloaded_url;
  extern const PreloadedLibrary preloaded_star;
  extern const PreloadedLibrary preloaded_cat;
  extern const PreloadedLibrary preloaded_http;
  extern const PreloadedLibrary preloaded_scheduler;
#ifndef __EMSCRIPTEN__
  extern const PreloadedLibrary preloaded_socket;
#endif
  extern const PreloadedLibrary preloaded_bigint;
  extern const PreloadedLibrary preloaded_xml;

  inline const PreloadedLibrary* const all_preloaded[] = {
    &preloaded_crypto,
    &preloaded_json,
    &preloaded_base32,
    &preloaded_base64,
    &preloaded_assert,
    &preloaded_vector3,
    &preloaded_url,
    &preloaded_star,
    &preloaded_cat,
    &preloaded_http,
    &preloaded_scheduler,
#ifndef __EMSCRIPTEN__
    &preloaded_socket,
#endif
    &preloaded_bigint,
    &preloaded_xml,
  };
}

LUAMOD_API int (luaopen_crypto)    (lua_State *L);
LUAMOD_API int (luaopen_json)      (lua_State *L);
LUAMOD_API int (luaopen_base32)    (lua_State *L);
LUAMOD_API int (luaopen_base64)    (lua_State *L);
LUAMOD_API int (luaopen_assert)    (lua_State *L);
LUAMOD_API int (luaopen_vector3)   (lua_State *L);
LUAMOD_API int (luaopen_url)       (lua_State *L);
LUAMOD_API int (luaopen_star)      (lua_State *L);
LUAMOD_API int (luaopen_cat)       (lua_State *L);
LUAMOD_API int (luaopen_http)      (lua_State *L);
LUAMOD_API int (luaopen_scheduler) (lua_State *L);
#ifndef __EMSCRIPTEN__
LUAMOD_API int (luaopen_socket)    (lua_State *L);
#endif
LUAMOD_API int (luaopen_bigint)    (lua_State *L);
LUAMOD_API int (luaopen_xml)       (lua_State *L);

/* open all previous libraries */
LUALIB_API void (luaL_openlibs) (lua_State *L);

/* utility for implementation of "universal" variants */
#define pluto_uwrap(L, f, r) \
  auto old = setlocale(LC_ALL, nullptr); \
  setlocale(LC_NUMERIC, "en_US.UTF-8"); \
  int narg = lua_gettop(L); \
  lua_pushcfunction(L, f); \
  lua_insert(L, 1); \
  int status = lua_pcall(L, narg, r, 0); \
  if (status != LUA_OK) { \
    setlocale(LC_ALL, old); \
    lua_error(L); \
  } \
  setlocale(LC_ALL, old); \
  return r;
