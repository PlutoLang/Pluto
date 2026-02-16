#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"
#include "llimits.h"

#include "vendor/Soup/soup/MemoryRefReader.hpp"
#include "vendor/Soup/soup/wasm.hpp"

static thread_local lua_State* callback_L = nullptr;

static soup::WasmScript *checkmodule (lua_State *L, int i) {
  return (soup::WasmScript*)luaL_checkudata(L, i, "pluto:wasm-module");
}

static void wasm_to_lua_stack(lua_State *L, soup::WasmVm& vm, const std::vector<soup::WasmType>& types) {
  lua_checkstack(L, static_cast<int>(types.size()) + 1);
  const auto base = lua_gettop(L);
  for (auto i = static_cast<int>(types.size()); i-- != 0; ) {
    switch (types[i]) {
      case soup::WASM_I32: default:
        lua_pushinteger(L, vm.stack.top().i32);
        break;
      case soup::WASM_I64:
        lua_pushinteger(L, vm.stack.top().i64);
        break;
      case soup::WASM_F32:
        lua_pushnumber(L, vm.stack.top().f32);
        break;
      case soup::WASM_F64:
        lua_pushnumber(L, vm.stack.top().f64);
        break;
    }
    lua_insert(L, base + 1);
    vm.stack.pop();
  }
}

static int call (lua_State *L) {
  auto& inst = *checkmodule(L, 1);
  const soup::WasmScript::FunctionType* type;
  auto code = inst.getExportedFuntion(pluto_checkstring(L, 2), &type);
  if (l_unlikely(!code)) {
    return luaL_error(L, "function export not found");
  }
  lua_checkstack(L, static_cast<int>(3 + type->parameters.size()));  /* ensure enough space for parameters + vm */
  while (lua_gettop(L) < 2 + type->parameters.size()) {
    lua_pushnil(L);
  }
  auto& vm = *pluto_newclassinst(L, soup::WasmVm, inst);
  for (int i = 0; i != type->parameters.size(); ++i) {
    switch (type->parameters[i]) {
      case soup::WASM_I32: default:
        vm.locals.emplace_back(static_cast<int32_t>(luaL_optinteger(L, 3 + i, 0)));
        break;
      case soup::WASM_I64:
        vm.locals.emplace_back(static_cast<int64_t>(luaL_optinteger(L, 3 + i, 0)));
        break;
      case soup::WASM_F32:
        vm.locals.emplace_back(static_cast<float>(luaL_optnumber(L, 3 + i, 0.0)));
        break;
      case soup::WASM_F64:
        vm.locals.emplace_back(static_cast<double>(luaL_optnumber(L, 3 + i, 0.0)));
        break;
    }
  }
  callback_L = L;
  if (!vm.run(*code)
    || vm.stack.size() < type->results.size() // Not using != because the VM stack is over-filled due to implicit drops only happening on internal calls.
    ) {
    return luaL_error(L, "wasm vm error");
  }
  wasm_to_lua_stack(L, vm, type->results);
  return static_cast<int>(type->results.size());
}

static int wasm_module_read (lua_State *L) {
  auto& inst = *checkmodule(L, 1);
  lua_Unsigned addr = luaL_checkinteger(L, 2);
  lua_Unsigned size = luaL_checkinteger(L, 3);
  auto ptr = inst.memory.getView(addr, size);
  if (l_unlikely(!ptr)) {
    luaL_error(L, "attempt to read past end of memory");
  }
  lua_pushlstring(L, (const char*)ptr, size);
  return 1;
}

static int wasm_module_write (lua_State *L) {
  auto& inst = *checkmodule(L, 1);
  lua_Unsigned addr = luaL_checkinteger(L, 2);
  size_t size;
  const char *data = luaL_checklstring(L, 3, &size);
  auto ptr = inst.memory.getView(addr, size);
  if (l_unlikely(!ptr)) {
    luaL_error(L, "attempt to write past end of memory");
  }
  memcpy(ptr, data, size);
  return 0;
}

static soup::WasmScript *pushmodule (lua_State *L) {
  auto ptr = new (lua_newuserdata(L, sizeof(soup::WasmScript))) soup::WasmScript();
  if (l_unlikely(luaL_newmetatable(L, "pluto:wasm-module"))) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State* L) {
      std::destroy_at<>(checkmodule(L, 1));
      return 0;
    });
    lua_settable(L, -3);
    lua_pushliteral(L, "__index");
    lua_newtable(L);
    {
      lua_pushliteral(L, "call");
      lua_pushcfunction(L, &call);
      lua_settable(L, -3);
    }
    {
      lua_pushliteral(L, "read");
      lua_pushcfunction(L, &wasm_module_read);
      lua_settable(L, -3);
    }
    {
      lua_pushliteral(L, "write");
      lua_pushcfunction(L, &wasm_module_write);
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  return ptr;
}

static int instantiate (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  if (lua_gettop(L) < 2) {
    lua_newtable(L);
  }
  soup::MemoryRefReader r(data, size);
  auto& inst = *pushmodule(L);
  if (l_unlikely(!inst.load(r))) {
    return luaL_error(L, "failed to load wasm module");
  }
  for (size_t i = 0; i != inst.function_imports.size(); ++i) {
    auto& imp = inst.function_imports[i];
    pluto_pushstring(L, imp.module_name);
    if (l_unlikely(lua_gettable(L, 2) <= LUA_TNIL)) {
      return luaL_error(L, "missing import module: %s", imp.module_name.c_str());
    }
    pluto_pushstring(L, imp.function_name);
    if (l_unlikely(lua_gettable(L, -2) <= LUA_TNIL)) {
      return luaL_error(L, R"(missing import function "%s" in module "%s")", imp.function_name.c_str(), imp.module_name.c_str());
    }
    lua_pushinteger(L, reinterpret_cast<uintptr_t>(&inst) + i);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
    lua_pop(L, 2);
    imp.ptr = [](soup::WasmVm& vm, uint32_t func_index) {
      lua_assert(callback_L);
      auto& func = vm.script.function_imports[func_index];
      auto& type = vm.script.types[func.type_index];
      const auto L = callback_L;
      lua_pushinteger(L, reinterpret_cast<uintptr_t>(&vm.script) + func_index);
      lua_gettable(L, LUA_REGISTRYINDEX);
      wasm_to_lua_stack(L, vm, type.parameters);
      lua_call(L, static_cast<int>(type.parameters.size()), static_cast<int>(type.results.size()));
      for (int i = 0; i != type.results.size(); ++i) {
        switch (type.results[i]) {
          case soup::WASM_I32: default:
            vm.stack.emplace(static_cast<int32_t>(luaL_optinteger(L, (static_cast<int>(type.results.size()) - i) * -1, 0)));
            break;
          case soup::WASM_I64:
            vm.stack.emplace(static_cast<int64_t>(luaL_optinteger(L, (static_cast<int>(type.results.size()) - i) * -1, 0)));
            break;
          case soup::WASM_F32:
            vm.stack.emplace(static_cast<float>(luaL_optnumber(L, (static_cast<int>(type.results.size()) - i) * -1, 0.0)));
            break;
          case soup::WASM_F64:
            vm.stack.emplace(static_cast<double>(luaL_optnumber(L, (static_cast<int>(type.results.size()) - i) * -1, 0.0)));
            break;
        }
      }
    };
  }
  callback_L = L;
  if (l_unlikely(!inst.instantiate())) {
    return luaL_error(L, "failed to instantiate wasm module");
  }
  return 1;
}

static const luaL_Reg funcs_wasm[] = {
  {"instantiate", instantiate},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(wasm)
