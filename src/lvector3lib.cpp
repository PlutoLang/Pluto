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

local vector3, xyzmap
local g_system

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

  static function setsystem(pos, rot)
    g_system = new xyzmap(pos, rot)
  end

  static function getsystem()
    return g_system:tostrings()
  end

  function torot(format)
    if format then
      local pos, rot
      if format[1] ~= "z" then
        pos = format[2] == "r" ? "-r+u+f" : "+r+u+f"
        rot = "+p+y+b"
      else
        pos = format[2] ~= "r" ? "-r+f+u" : "+r+f+u"
        rot = "+p+b+y"
      end
      warn("vector3.torot: specifying a format is deprecated; for \""..format.."\", call vector3.setsystem(\""..pos.."\", \""..rot.."\") instead")
      vector3.setsystem(pos, rot)
    end
    local f, r, u = g_system:unmappos(self)
    return g_system:maprot(
      math.deg(math.atan(r, f)),
      math.deg(math.asin(u / self:magnitude())),
      0
    )
  end

  function lookat(b, format)
    local dir = (b - self)
    return dir:torot(format)
  end

  function todir(format)
    if format then
      local pos, rot
      if format[1] ~= "z" then
        pos = format[2] == "r" ? "-r+u+f" : "+r+u+f"
        rot = "+p+y+b"
      else
        pos = format[2] ~= "r" ? "-r+f+u" : "+r+f+u"
        rot = "+p+b+y"
      end
      warn("vector3.todir: specifying a format is deprecated; for \""..format.."\", call vector3.setsystem(\""..pos.."\", \""..rot.."\") instead")
      vector3.setsystem(pos, rot)
    end
    local yaw, pitch = g_system:unmaprot(self)
    local yaw_radians = math.rad(yaw)
    local pitch_radians = math.rad(pitch)
    return g_system:mappos(
      math.cos(pitch_radians) * math.cos(yaw_radians),
      math.cos(pitch_radians) * math.sin(yaw_radians),
      math.sin(pitch_radians)
    )
  end
end

setmetatable(vector3, {
  function __call(x, y, z)
    return new vector3(x, y, z)
  end
})

class xyzmap
  function __construct(pos, rot)
    assert(#pos == 6, "invalid position system specification")
    assert(pos[1] == "+" or pos[1] == "-", "invalid position system specification")
    assert(pos[2] == "f" or pos[2] == "r" or pos[2] == "u", "invalid position system specification")
    assert(pos[3] == "+" or pos[3] == "-", "invalid position system specification")
    assert(pos[4] == "f" or pos[4] == "r" or pos[4] == "u", "invalid position system specification")
    assert(pos[5] == "+" or pos[5] == "-", "invalid position system specification")
    assert(pos[6] == "f" or pos[6] == "r" or pos[6] == "u", "invalid position system specification")

    assert(#rot == 6, "invalid rotation system specification")
    assert(rot[1] == "+" or rot[1] == "-", "invalid rotation system specification")
    assert(rot[2] == "y" or rot[2] == "p" or rot[2] == "b", "invalid rotation system specification")
    assert(rot[3] == "+" or rot[3] == "-", "invalid rotation system specification")
    assert(rot[4] == "y" or rot[4] == "p" or rot[4] == "b", "invalid rotation system specification")
    assert(rot[5] == "+" or rot[5] == "-", "invalid rotation system specification")
    assert(rot[6] == "y" or rot[6] == "p" or rot[6] == "b", "invalid rotation system specification")

    self[pos[2]] = "x"
    self[pos[2] .. "f"] = pos[1] == "+" ? +1 : -1
    self[pos[4]] = "y"
    self[pos[4] .. "f"] = pos[3] == "+" ? +1 : -1
    self[pos[6]] = "z"
    self[pos[6] .. "f"] = pos[5] == "+" ? +1 : -1

    self[rot[2]] = "x"
    self[rot[2] .. "f"] = rot[1] == "+" ? +1 : -1
    self[rot[4]] = "y"
    self[rot[4] .. "f"] = rot[3] == "+" ? +1 : -1
    self[rot[6]] = "z"
    self[rot[6] .. "f"] = rot[5] == "+" ? +1 : -1

    assert(self.f and self.r and self.u, "invalid position system specification")
    assert(self.y and self.p and self.b, "invalid rotation system specification")
  end

  function mappos(f, r, u)
    local vec = new vector3()
    vec[self.f] = f * self.ff
    vec[self.r] = r * self.rf
    vec[self.u] = u * self.uf
    return vec
  end

  function unmappos(vec)
    return vec[self.f] * self.ff, vec[self.r] * self.rf, vec[self.u] * self.uf
  end

  function maprot(y, p, b)
    local vec = new vector3()
    vec[self.y] = y * self.yf
    vec[self.p] = p * self.pf
    vec[self.b] = b * self.bf
    return vec
  end

  function unmaprot(vec)
    return vec[self.y] * self.yf, vec[self.p] * self.pf, vec[self.b] * self.bf
  end

  function tostrings()
    local pos = {}
    pos[self.f .. "s"] = self.ff > 0 ? "+" : "-"
    pos[self.f] = "f"
    pos[self.r .. "s"] = self.rf > 0 ? "+" : "-"
    pos[self.r] = "r"
    pos[self.u .. "s"] = self.uf > 0 ? "+" : "-"
    pos[self.u] = "u"

    local rot = {}
    rot[self.y .. "s"] = self.yf > 0 ? "+" : "-"
    rot[self.y] = "y"
    rot[self.p .. "s"] = self.pf > 0 ? "+" : "-"
    rot[self.p] = "p"
    rot[self.b .. "s"] = self.bf > 0 ? "+" : "-"
    rot[self.b] = "b"

    return pos.xs .. pos.x .. pos.ys .. pos.y .. pos.zs .. pos.z, rot.xs .. rot.x .. rot.ys .. rot.y .. rot.zs .. rot.z
  end
end

vector3.setsystem("+r+u+f", "+p+y+b")

return vector3)EOC";
    luaL_loadbuffer(L, code, strlen(code), "pluto:vector3");
    lua_call(L, 0, 1);
    return 1;
#endif
}

const Pluto::PreloadedLibrary Pluto::preloaded_vector3{ PLUTO_VECTOR3LIBNAME, funcs_vector3, &luaopen_vector3 };
