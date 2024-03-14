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


/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const luaL_Reg loadedlibs[] = {
  {LUA_GNAME, luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
#ifndef PLUTO_NO_COROLIB
  {LUA_COLIBNAME, luaopen_coroutine},
#endif
  {LUA_TABLIBNAME, luaopen_table},
#ifndef PLUTO_NO_FILESYSTEM
  {LUA_IOLIBNAME, luaopen_io},
#endif
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
#ifndef PLUTO_NO_DEBUGLIB
  {LUA_DBLIBNAME, luaopen_debug},
#endif
  {NULL, NULL}
};


LUALIB_API void luaL_openlibs (lua_State *L) {
  const luaL_Reg *lib;

  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }

  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
  for (const Pluto::PreloadedLibrary* lib : Pluto::all_preloaded) {
    lua_pushcfunction(L, lib->init);
    lua_setfield(L, -2, lib->name);
  }
  lua_pop(L, 1);

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
}

