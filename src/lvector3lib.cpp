#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs_vector3[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_vector3(lua_State* L) {
#ifdef PLUTO_DONT_LOAD_ANY_STANDARD_LIBRARY_CODE_WRITTEN_IN_PLUTO
    return 0;
#else
    const auto code = R"EOC(pluto_use "0.6.0"

local vector3
 class vector3
  __name = "pluto:vector3"

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
    if b instanceof vector3 then
      return new vector3(self.x + b.x, self.y + b.y, self.z + b.z)
    end
    return new vector3(self.x + b, self.y + b, self.z + b)
  end

  function __sub(b)
    if b instanceof vector3 then
      return new vector3(self.x - b.x, self.y - b.y, self.z - b.z)
    end
    return new vector3(self.x - b, self.y - b, self.z - b)
  end

  function __mul(b)
    if b instanceof vector3 then
      return new vector3(self.x * b.x, self.y * b.y, self.z * b.z)
    end
    return new vector3(self.x * b, self.y * b, self.z * b)
  end

  function __div(b)
    if b instanceof vector3 then
      return new vector3(self.x / b.x, self.y / b.y, self.z / b.z)
    end
    return new vector3(self.x / b, self.y / b, self.z / b)
  end

  function __eq(b)
    return self.x == b.x and self.y == b.y and self.z == b.z
  end

  function __len()
    return self:magnitude()
  end

  function __tostring()
    return $"vector3({self.x}, {self.y}, {self.z})"
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
    return new vector3(
      self.y * b.z - self.z * b.y,
      self.z * b.x - self.x * b.z,
      self.x * b.y - self.y * b.x
    )
  end

  function abs()
    return new vector3(math.abs(self.x), math.abs(self.y), math.abs(self.z))
  end

  function normalised()
    return self / self:magnitude()
  end

  function normalized()
    return self / self:magnitude()
  end

  function torot(format)
    format ??= ""
    if format[1] ~= 'z' then
      assert(format[1] == nil or format[1] == 'y', "Invalid up-axis in format")
      local yaw = math.deg(math.atan(self.x, self.z))
      if format[2] == 'r' then
        yaw *= -1
      end
      local pitch = math.deg(math.asin(self.y / self:magnitude()))
      return new vector3(
        math.isnan(pitch) ? 0 : pitch,
        yaw,
        0
      )
    else
      local yaw = math.deg(math.atan(self.x, self.y))
      if format[2] ~= 'r' then
        yaw *= -1
      end
      local pitch = math.deg(math.asin(self.z / self:magnitude()))
      return new vector3(
        math.isnan(pitch) ? 0 : pitch,
        0,
        yaw
      )
    end
  end

  function lookat(b, format)
    local dir = (b - self)
    return dir:torot(format)
  end

  function todir(format)
    format ??= ""
    if format[1] ~= 'z' then
      assert(format[1] == nil or format[1] == 'y', "Invalid up-axis in format")
      local handedness_factor = (format[2] == 'r' ? -1 : +1)
      local yaw_radians = math.rad(self.y)
      local pitch_radians = math.rad(self.x) * handedness_factor
      return new vector3(
        math.cos(pitch_radians) * math.sin(yaw_radians) * handedness_factor,
        math.sin(pitch_radians) * handedness_factor,
        math.cos(pitch_radians) * math.cos(yaw_radians)
      )
    else
      local handedness_factor = (format[2] ~= 'r' ? -1 : +1)
      local yaw_radians = math.rad(self.z)
      local pitch_radians = math.rad(self.x) * handedness_factor
      return new vector3(
        math.cos(pitch_radians) * math.sin(yaw_radians) * handedness_factor,
        math.cos(pitch_radians) * math.cos(yaw_radians),
        math.sin(pitch_radians) * handedness_factor
      )
    end
  end
end

setmetatable(vector3, {
  function __call(x, y, z)
    return new vector3(x, y, z)
  end
})

return vector3)EOC";
    luaL_loadbuffer(L, code, strlen(code), "pluto:vector3");
    lua_call(L, 0, 1);
    return 1;
#endif
}

const Pluto::PreloadedLibrary Pluto::preloaded_vector3{ "vector3", funcs_vector3, &luaopen_vector3 };
