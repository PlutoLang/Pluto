/*
** $Id: linit.c $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"


#include <stddef.h>
#include <cstring>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


#ifdef PLUTO_NO_COROLIB
#error PLUTO_NO_COROLIB is no longer supported as of Pluto 0.12.0. Use luaL_openselectedlibs with appropriate bitflags, instead.
#endif

#ifdef PLUTO_NO_DEBUGLIB
#error PLUTO_NO_DEBUGLIB is no longer supported as of Pluto 0.12.0. Use luaL_openselectedlibs with appropriate bitflags, instead.
#endif


/*
** Standard Libraries. (Must be listed in the same ORDER of their
** respective constants LUA_<libname>K.)
*/
static const luaL_Reg stdlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_DBLIBNAME, luaopen_debug},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {PLUTO_CRYPTOLIBNAME, luaopen_crypto},
  {PLUTO_JSONLIBNAME, luaopen_json},
  {PLUTO_BASE32LIBNAME, luaopen_base32},
  {PLUTO_BASE64LIBNAME, luaopen_base64},
  {PLUTO_ASSERTLIBNAME, luaopen_assert},
  {PLUTO_VECTOR3LIBNAME, luaopen_vector3},
  {PLUTO_URLLIBNAME, luaopen_url},
  {PLUTO_STARLIBNAME, luaopen_star},
  {PLUTO_CATLIBNAME, luaopen_cat},
  {PLUTO_HTTPLIBNAME, luaopen_http},
  {PLUTO_SCHEDULERLIBNAME, luaopen_scheduler},
  {PLUTO_BIGINTLIBNAME, luaopen_bigint},
  {PLUTO_XMLLIBNAME, luaopen_xml},
  {PLUTO_REGEXLIBNAME, luaopen_regex},
  {PLUTO_FFILIBNAME, luaopen_ffi},
  {PLUTO_CANVASLIBNAME, luaopen_canvas},
  {PLUTO_BUFFERLIBNAME, luaopen_buffer},
#ifndef __EMSCRIPTEN__
  {PLUTO_SOCKETLIBNAME, luaopen_socket},
#endif
  {NULL, NULL}
};


/*
** require and preload selected standard libraries
*/
LUALIB_API void luaL_openselectedlibs (lua_State *L, int load, int preload) {
  int mask;
  const luaL_Reg *lib;
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  for (lib = stdlibs, mask = 1; lib->name != NULL; lib++, mask <<= 1) {
    if (load & mask) {  /* selected? */
      luaL_requiref(L, lib->name, lib->func, 1);  /* require library */
      lua_pop(L, 1);  /* remove result from the stack */
    }
    else if (preload & mask) {  /* selected? */
      lua_pushcfunction(L, lib->func);
      lua_setfield(L, -2, lib->name);  /* add library to PRELOAD table */
    }
  }
  //lua_assert((mask >> 1) == LUA_UTF8LIBK);
  lua_pop(L, 1);  /* remove PRELOAD table */

#ifndef PLUTO_DONT_LOAD_ANY_STANDARD_LIBRARY_CODE_WRITTEN_IN_PLUTO
  const auto startup_code = R"EOC(
pluto_use "0.6.0"

class exception
    __name = "pluto:exception"

    function __construct(public what)
        local caller
        local i = 2
        while true do
            caller = debug.getinfo(i)
            if caller == nil then
                error("exception instances must be created with 'pluto_new'", 0)
            end
            ++i
            if caller.name == "Pluto_operator_new" then
                caller = debug.getinfo(i)
                break
            end
        end
        self.where = $"{caller.short_src}:{caller.currentline}"
        error(self, 0)
    end

    function __tostring()
        return $"{self.where}: {tostring(self.what)}"
    end
end

function instanceof(a, b)
  return a instanceof b
end
)EOC";
  luaL_loadbuffer(L, startup_code, strlen(startup_code), "Pluto Standard Library");
  lua_call(L, 0, 0);
#endif
}

