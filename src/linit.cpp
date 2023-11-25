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
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
#ifndef PLUTO_NO_FILESYSTEM
  {LUA_IOLIBNAME, luaopen_io},
#endif
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_DBLIBNAME, luaopen_debug},
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

class Exception
    __name = "Exception"

    function __construct(public what)
        local caller
        local i = 2
        while true do
            caller = debug.getinfo(i)
            if caller == nil then
                error("Exception instances must be created with 'pluto_new'", 0)
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

package.preload["Vector3"] = function()
  local Vector3
   class Vector3
    __name = "Vector3"

    function __construct(x, y, z)
      if x ~= nil and y == nil and z == nil then
        -- (a) -> (a, a, a)
        self.x = x
        self.y = x
        self.z = x
      else
        -- (a, b) -> (a, b, 0)
        -- (a, b, c) -> (a, b, c)
        self.x = x ?? 0
        self.y = y ?? 0
        self.z = z ?? 0
      end
    end

    function __add(b)
      if b instanceof Vector3 then
        return new Vector3(self.x + b.x, self.y + b.y, self.z + b.z)
      end
      return new Vector3(self.x + b, self.y + b, self.z + b)
    end

    function __sub(b)
      if b instanceof Vector3 then
        return new Vector3(self.x - b.x, self.y - b.y, self.z - b.z)
      end
      return new Vector3(self.x - b, self.y - b, self.z - b)
    end

    function __mul(b)
      if b instanceof Vector3 then
        return new Vector3(self.x * b.x, self.y * b.y, self.z * b.z)
      end
      return new Vector3(self.x * b, self.y * b, self.z * b)
    end

    function __div(b)
      if b instanceof Vector3 then
        return new Vector3(self.x / b.x, self.y / b.y, self.z / b.z)
      end
      return new Vector3(self.x / b, self.y / b, self.z / b)
    end

    function __eq(b)
      return self.x == b.x and self.y == b.y and self.z == b.z
    end

    function __len()
      return self:magnitude()
    end

    function __tostring()
      return $"Vector3({self.x}, {self.y}, {self.z})"
    end

    function magnitude()
      local accum = 0
      for self as axis do
        accum += axis^2
      end
      return math.sqrt(accum)
    end

    function distance(b)
      return (self - b):magnitude()
    end

    function sum()
      local accum = 0
      for self as axis do
        accum += axis
      end
      return accum
    end

    function min()
      local min = math.huge
      for self as axis do
        if min > axis then
          min = axis
        end
      end
      return min
    end

    function max()
      local max = 0
      for self as axis do
        if max < axis then
          max = axis
        end
      end
      return max
    end

    function dot(b)
      return (self * b):sum()
    end

    function crossproduct(b)
      return new Vector3(
        self.y * b.z - self.z * b.y,
        self.z * b.x - self.x * b.z,
        self.x * b.y - self.y * b.x
      )
    end

    function abs()
      return new Vector3(math.abs(self.x), math.abs(self.y), math.abs(self.z))
    end

    function normalised()
      return self / self:magnitude()
    end

    function normalized()
      return self / self:magnitude()
    end

    function torot(up)
      if up == "y" then
        local yaw = math.deg(math.atan(self.x, self.z)) * -1
        local pitch = math.deg(math.asin(self.y / self:magnitude()))
        return new Vector3(
          math.isnan(pitch) ? 0 : pitch,
          yaw,
          0
        )
      elseif up == "z" then
        local yaw = math.deg(math.atan(self.x, self.y)) * -1
        local pitch = math.deg(math.asin(self.z / self:magnitude()))
        return new Vector3(
          math.isnan(pitch) ? 0 : pitch,
          0,
          yaw
        )
      else
        error("Expected \"y\" or \"z\" for 'up' parameter")
      end
    end

    function lookat(up)
      local dir = (b - self)
      return dir:torot(up)
    end

    function todir(up)
      if up == "y" then
        local yaw_radians = math.rad(self.z)
        local pitch_radians = math.rad(self.x) * -1
        return new Vector3(
          math.cos(pitch_radians) * math.sin(yaw_radians) * -1,
          math.sin(pitch_radians) * -1,
          math.cos(pitch_radians) * math.cos(yaw_radians)
        )
      elseif up == "z" then
        local yaw_radians = math.rad(self.z)
        local pitch_radians = math.rad(self.x) * -1
        return new Vector3(
          math.cos(pitch_radians) * math.sin(yaw_radians) * -1,
          math.cos(pitch_radians) * math.cos(yaw_radians),
          math.sin(pitch_radians) * -1
        )
      else
        error("Expected \"y\" or \"z\" for 'up' parameter")
      end
    end
  end

  setmetatable(Vector3, {
    function __call(x, y, z)
      return new Vector3(x, y, z)
    end
  })

  return Vector3
end
)EOC";
  luaL_loadbuffer(L, startup_code, strlen(startup_code), "Pluto Standard Library");
  lua_call(L, 0, 0);
}

