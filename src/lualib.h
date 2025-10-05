/*
** $Id: lualib.h $
** Lua standard libraries
** See Copyright Notice in lua.h
*/


#ifndef lualib_h
#define lualib_h

#include "lua.h"
#include "lauxlib.h" // Pluto::Preloaded


/* version suffix for environment variable names */
#define LUA_VERSUFFIX          "_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR


#define LUA_GLIBK		1
LUAMOD_API int (luaopen_base) (lua_State *L);

#define LUA_LOADLIBNAME	"package"
#define LUA_LOADLIBK	(LUA_GLIBK << 1)
LUAMOD_API int (luaopen_package) (lua_State *L);

#define LUA_COLIBNAME	"coroutine"
#define LUA_COLIBK	(LUA_LOADLIBK << 1)
LUAMOD_API int (luaopen_coroutine) (lua_State *L);

#define LUA_DBLIBNAME	"debug"
#define LUA_DBLIBK	(LUA_COLIBK << 1)
LUAMOD_API int (luaopen_debug) (lua_State *L);

#define LUA_IOLIBNAME	"io"
#define LUA_IOLIBK	(LUA_DBLIBK << 1)
LUAMOD_API int (luaopen_io) (lua_State *L);

#define LUA_MATHLIBNAME	"math"
#define LUA_MATHLIBK	(LUA_IOLIBK << 1)
LUAMOD_API int (luaopen_math) (lua_State *L);

#define LUA_OSLIBNAME	"os"
#define LUA_OSLIBK	(LUA_MATHLIBK << 1)
LUAMOD_API int (luaopen_os) (lua_State *L);

#define LUA_STRLIBNAME	"string"
#define LUA_STRLIBK	(LUA_OSLIBK << 1)
LUAMOD_API int (luaopen_string) (lua_State *L);

#define LUA_TABLIBNAME	"table"
#define LUA_TABLIBK	(LUA_STRLIBK << 1)
LUAMOD_API int (luaopen_table) (lua_State *L);

#define LUA_UTF8LIBNAME	"utf8"
#define LUA_UTF8LIBK	(LUA_TABLIBK << 1)
LUAMOD_API int (luaopen_utf8) (lua_State *L);


#define PLUTO_DEFAULTLOADLIBS PLUTO_CRYPTOLIBK - 1

#define PLUTO_CRYPTOLIBNAME "crypto"
#define PLUTO_CRYPTOLIBK (LUA_UTF8LIBK << 1)
LUAMOD_API int (luaopen_crypto)	(lua_State *L);

#define PLUTO_JSONLIBNAME "json"
#define PLUTO_JSONLIBK (PLUTO_CRYPTOLIBK << 1)
LUAMOD_API int (luaopen_json)	(lua_State *L);

#define PLUTO_BASE32LIBNAME "base32"
#define PLUTO_BASE32LIBK (PLUTO_JSONLIBK << 1)
LUAMOD_API int (luaopen_base32)	(lua_State *L);

#define PLUTO_BASE64LIBNAME "base64"
#define PLUTO_BASE64LIBK (PLUTO_BASE32LIBK << 1)
LUAMOD_API int (luaopen_base64)	(lua_State *L);

#define PLUTO_ASSERTLIBNAME "assert"
#define PLUTO_ASSERTLIBK (PLUTO_BASE64LIBK << 1)
LUAMOD_API int (luaopen_assert)	(lua_State *L);

#define PLUTO_VECTOR3LIBNAME "vector3"
#define PLUTO_VECTOR3LIBK (PLUTO_ASSERTLIBK << 1)
LUAMOD_API int (luaopen_vector3)	(lua_State *L);

#define PLUTO_URLLIBNAME "url"
#define PLUTO_URLLIBK (PLUTO_VECTOR3LIBK << 1)
LUAMOD_API int (luaopen_url)	(lua_State *L);

#define PLUTO_STARLIBNAME "*"
#define PLUTO_STARLIBK (PLUTO_URLLIBK << 1)
LUAMOD_API int (luaopen_star)	(lua_State *L);

#define PLUTO_CATLIBNAME "cat"
#define PLUTO_CATLIBK (PLUTO_STARLIBK << 1)
LUAMOD_API int (luaopen_cat)	(lua_State *L);

#define PLUTO_HTTPLIBNAME "http"
#define PLUTO_HTTPLIBK (PLUTO_CATLIBK << 1)
LUAMOD_API int (luaopen_http)	(lua_State *L);

#define PLUTO_SCHEDULERLIBNAME "scheduler"
#define PLUTO_SCHEDULERLIBK (PLUTO_HTTPLIBK << 1)
LUAMOD_API int (luaopen_scheduler)	(lua_State *L);

#define PLUTO_BIGINTLIBNAME "bigint"
#define PLUTO_BIGINTLIBK (PLUTO_SCHEDULERLIBK << 1)
LUAMOD_API int (luaopen_bigint)	(lua_State *L);

#define PLUTO_XMLLIBNAME "xml"
#define PLUTO_XMLLIBK (PLUTO_BIGINTLIBK << 1)
LUAMOD_API int (luaopen_xml)	(lua_State *L);

#define PLUTO_REGEXLIBNAME "regex"
#define PLUTO_REGEXLIBK (PLUTO_XMLLIBK << 1)
LUAMOD_API int (luaopen_regex)	(lua_State *L);

#define PLUTO_FFILIBNAME "ffi"
#define PLUTO_FFILIBK (PLUTO_REGEXLIBK << 1)
LUAMOD_API int (luaopen_ffi)	(lua_State *L);

#define PLUTO_CANVASLIBNAME "canvas"
#define PLUTO_CANVASLIBK (PLUTO_FFILIBK << 1)
LUAMOD_API int (luaopen_canvas)	(lua_State *L);

#define PLUTO_BUFFERLIBNAME "buffer"
#define PLUTO_BUFFERLIBK (PLUTO_CANVASLIBK << 1)
LUAMOD_API int (luaopen_buffer)	(lua_State *L);

#ifndef __EMSCRIPTEN__
#define PLUTO_SOCKETLIBNAME "socket"
#define PLUTO_SOCKETLIBK (PLUTO_BUFFERLIBK << 1)
LUAMOD_API int (luaopen_socket)(lua_State* L);
#endif


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
  extern const PreloadedLibrary preloaded_bigint;
  extern const PreloadedLibrary preloaded_xml;
  extern const PreloadedLibrary preloaded_regex;
  extern const PreloadedLibrary preloaded_ffi;
  extern const PreloadedLibrary preloaded_canvas;
  extern const PreloadedLibrary preloaded_buffer;
#ifndef __EMSCRIPTEN__
  extern const PreloadedLibrary preloaded_socket;
#endif

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
    &preloaded_bigint,
    &preloaded_xml,
    &preloaded_regex,
    &preloaded_ffi,
    &preloaded_canvas,
    &preloaded_buffer,
#ifndef __EMSCRIPTEN__
    &preloaded_socket,
#endif
  };

  extern const ConstexprLibrary constexpr_io;

  inline const ConstexprLibrary* const all_constexpr[] = {
    &constexpr_io,
  };
}


/* open selected libraries */
LUALIB_API void (luaL_openselectedlibs)(lua_State* L, int load, int preload);

/* open all libraries */
#define luaL_openlibs(L)	luaL_openselectedlibs(L, PLUTO_DEFAULTLOADLIBS, ~0)

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


#endif
