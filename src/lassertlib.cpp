#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_assert(lua_State *L) {
#ifdef PLUTO_DONT_LOAD_ANY_STANDARD_LIBRARY_CODE_WRITTEN_IN_PLUTO
  return 0;
#else
  const auto code = R"EOC(pluto_use "0.6.0"

local module = {}

local function deepCompare(t1, t2)
  if t1 == t2 then
    return true
  end

  if #t1 ~= #t2 then
    return false
  end

  if next(t1) == nil and next(t2) ~= nil then
    return false
  end

  for k, v1 in t1 do
    local v2 = t2[k]
      
    if type(v1) == "table" and type(v2) == "table" and v1 ~= v2 then
      if not deepCompare(v1, v2) then
        return false
      end
    elseif v1 ~= v2 then
      return false
    end
  end

  for k in t2 do
    if t1[k] == nil then
      return false
    end
  end

  return true
end

local class AssertionError
  function __construct(assertion_name: string, public intended_value, public received_value)
    self.assertion_name = "assert." .. assertion_name
    self.should_dump = true
    self.write_intended = true
    self.write_recieved = true

    self.intended_name = "Intended Value"
    self.received_name = "Received Value"

    return self
  end

  function raise()
    local message = self:getFileInformation() .. " -> Assertion Error: " .. self:getExtraInformation()

    if self.write_intended then
      message ..= "\n\t"
      message ..= self.intended_name .. ": " .. self:dumpValue(self.intended_value, self.hardcoded ? false : self.should_dump)
    end

    if self.write_recieved then
      message ..= "\n\t"
      message ..= self.received_name .. ": " .. self:dumpValue(self.received_value, self.should_dump)
    end

    if self.operator_comparison then
      local n1 = self:dumpValue(self.intended_value)
      local n2 = self:dumpValue(self.received_value)
      local op = self.operator_comparison_operator

      message ..= "\n\t"
      message ..= $"Expression: ({n1} {op} {n2}) == false"
    end

    if self.simple then
      message ..= "\n\t"
      message ..= self.simple_message
    end

    message ..= "\n" -- Before traceback.

    error(message, 0)
  end

  function dumpValue(value, should_dump: bool = self.should_dump): string
    if type(value) == "function" or should_dump == false then
      return tostring(value)
    end

    local dump = dumpvar(value) -- dumpvar will still serialize subfunctions, need to fix that.

    if type(value) == "table" then
      if dump:len() < 80 then
        dump = dump:replace("\n", " "):replace("\t", "")
      else
        dump = "\n" .. dump:truncate(300, true)
      end
    end

    return dump
  end

  function getFileInformation(): string
    local caller

    for i = 2, 255 do
      caller = debug.getinfo(i)
      if caller and not tostring(caller.short_src):contains("[") then
        break
      end
    end

    local short_src = caller?.short_src ?? "unk"
    local currentline = caller?.currentline ?? "0"

    return $"{short_src}:{currentline}"
  end

  function getExtraInformation(): string
    return $"({self.assertion_name})"
  end

  function setDontDump() -- Applies to both the intended and received values.
    self.should_dump = false

    return self
  end

  function setHardcoded() -- Prevents 'Intended Value' from being dumped.
    self.hardcoded = true

    return self
  end

  function setOperatorComparison(operator: string) -- Expression: N operator N (replaces intended/received)
    self.operator_comparison = true
    self.operator_comparison_operator = operator
    self.write_intended = false
    self.write_recieved = false

    return self
  end

  function setSimple(message: string)
    self.write_recieved = false
    self.write_intended = false
    self.simple = true
    self.simple_message = message

    return self
  end

  function setNameOverride(intended: string, received: string)
    if intended then
      self.intended_name = intended
    end

    if received then
      self.received_name = received
    end

    return self
  end
end

function module.isnil(value)
  if value ~= nil then
    return new AssertionError("isnil", "nil", value):setHardcoded():raise()
  end
end

function module.istrue(value)
  if value ~= true then
    return new AssertionError("istrue", "true", value):setHardcoded():raise()
  end
end

function module.isfalse(value)
  if value ~= false then
    return new AssertionError("isfalse", "false", value):setHardcoded():raise()
  end
end

function module.falsy(value)
  if value then -- It's truthy
    return new AssertionError("falsy", "nil or false", value):setHardcoded():raise()
  end
end

function module.truthy(value)
  if not value then
    return new AssertionError("truthy", "not nil or false", value):setHardcoded():raise()
  end
end

 function module.notnil(value)
  if value == nil then
    return new AssertionError("notnil", "not nil", value):setHardcoded():raise()
  end
end

function module.equal(value1, value2)
  if type(value1) == "table" and type(value2) == "table" then
    if not deepCompare(value1, value2) then
      return new AssertionError("equal", value1, value2):setNameOverride("Value 1", "Value 2"):raise()
    end
  else
    if value1 ~= value2 then
      return new AssertionError("equal", value1, value2):setNameOverride("Value 1", "Value 2"):raise()
    end
  end
end

function module.nequal(value1, value2)
  if type(value1) == "table" and type(value2) == "table" then
    if deepCompare(value1, value2) then
      return new AssertionError("nequal", value1, value2):setNameOverride("Value 1", "Value 2"):raise()
    end
  else
    if value1 == value2 then
      return new AssertionError("nequal", value1, value2):setNameOverride("Value 1", "Value 2"):raise()
    end
  end
end

function module.less(value1, value2)
  if not (value1 < value2) then
    return new AssertionError("less", value1, value2):setOperatorComparison("<"):raise()
  end
end

function module.lesseq(value1, value2)
  if not (value1 <= value2) then
    return new AssertionError("lesseq", value1, value2):setOperatorComparison("<="):raise()
  end
end

function module.greater(value1, value2)
  if not (value1 > value2) then
    return new AssertionError("greater", value1, value2):setOperatorComparison(">"):raise()
  end
end

function module.greatereq(value1, value2)
  if not (value1 >= value2) then
    return new AssertionError("greatereq", value1, value2):setOperatorComparison(">="):raise()
  end
end

function module.noerror(callback, ...)
  local status, err = pcall(callback, ...)
  if status == false then
    return new AssertionError("noerror", nil, nil):setSimple("An error was raised: " .. tostring(err)):raise()
  end
end

function module.haserror(callback, ...)
  local status = pcall(callback, ...)
  if status == true then
    return new AssertionError("haserror", nil, nil):setSimple("Expected an error, but there was none."):raise()
  end
end

function module.searcherror(substring, callback, ...)
  local status, err = pcall(callback, ...)
  if status == true then
    return new AssertionError("searcherror", nil, nil):setSimple("Expected an error, but there was none."):raise()
  elseif not tostring(err):contains(substring) then
    return new AssertionError("searcherror", substring, tostring(err)):setDontDump():setNameOverride("Absent String", "Error Message"):raise()
  end
end

function module.type(desired_type, ...)
  local t = {...}
  for i = 1, #t do
    local v = t[i]
    if type(v) ~= desired_type then
      return new AssertionError($"type, argument #{i}", desired_type, v):setHardcoded():setNameOverride("Intended Type"):raise()
    end
  end
end

function module.contains(element, container)
  local container_type = type(container)
  if (container_type != "string" and container_type != "table") or (not element in container) then
    return new AssertionError("contains", element, container):setNameOverride("Element", "Container"):raise()
  end
end

setmetatable(module, {
  __call = function (self, cond, err_msg = "assertion failed!")
    err_msg = " " .. err_msg
    if not cond then
      return new AssertionError("assert", nil, nil):setSimple(err_msg):raise()
    end
  end
})

module.AssertionError = AssertionError

return module)EOC";
  luaL_loadbuffer(L, code, strlen(code), "pluto:assert");
  lua_call(L, 0, 1);
  return 1;
#endif
}

const Pluto::PreloadedLibrary Pluto::preloaded_assert{ "assert", funcs, &luaopen_assert };
