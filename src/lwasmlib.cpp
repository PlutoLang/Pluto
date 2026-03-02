#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"
#include "llimits.h"

#include "vendor/Soup/soup/MemoryRefReader.hpp"
#include "vendor/Soup/soup/wasm.hpp"

static thread_local lua_State* callback_L = nullptr;

static soup::WasmSharedEnvironment::ScriptRaii *checkmoduleraw (lua_State *L, int i) {
  return (soup::WasmSharedEnvironment::ScriptRaii*)luaL_checkudata(L, i, "pluto:wasm-module");
}

static soup::WasmScript& checkmodule (lua_State *L, int i) {
  return **checkmoduleraw(L, i);
}

static void pushfuncref (lua_State *L, const soup::WasmSharedEnvironment::FuncRef& funcref);
static void wasm_to_lua_stack (lua_State *L, soup::WasmSharedEnvironment& shared_env, std::vector<soup::WasmValue>& stack, const std::vector<soup::WasmType>& types) {
  lua_checkstack(L, static_cast<int>(types.size()) + 1);
  const auto base = lua_gettop(L);
  for (auto i = static_cast<int>(types.size()); i-- != 0; ) {
    switch (types[i]) {
      case soup::WASM_I32:
        lua_pushinteger(L, stack.back().i32);
        break;
      case soup::WASM_I64:
        lua_pushinteger(L, stack.back().i64);
        break;
      case soup::WASM_F32:
        lua_pushnumber(L, stack.back().f32);
        break;
      case soup::WASM_F64:
        lua_pushnumber(L, stack.back().f64);
        break;
      case soup::WASM_FUNCREF:
        if (stack.back().i64) {
          pushfuncref(L, shared_env.getFuncRef(stack.back().i64));
        }
        else {
          lua_pushnil(L);
        }
        break;
      case soup::WASM_EXTERNREF:
        if (stack.back().i64) {
          lua_rawgeti(L, LUA_REGISTRYINDEX, stack.back().i64 & 0xffff'ffff);
        }
        else {
          lua_pushnil(L);
        }
        break;
      default:
        luaL_error(L, "unexpected wasm value type at language boundary. can only convert i32, i64, f32, f64, funcref, and externref.\n");
    }
    lua_insert(L, base + 1);
    stack.pop_back();
  }
}

static soup::WasmSharedEnvironment& getstatewasmenv (lua_State *L);
static void opt_wasm_value (lua_State *L, int i, soup::WasmValue& value) {
  switch (value.type) {
    case soup::WASM_I32:
      value.i32 = static_cast<int32_t>(luaL_optinteger(L, i, 0));
      break;
    case soup::WASM_I64:
      value.i64 = static_cast<int64_t>(luaL_optinteger(L, i, 0));
      break;
    case soup::WASM_F32:
      value.f32 = static_cast<float>(luaL_optnumber(L, i, 0.0));
      break;
    case soup::WASM_F64:
      value.f64 = static_cast<double>(luaL_optnumber(L, i, 0.0));
      break;
    case soup::WASM_EXTERNREF:
      if (lua_type(L, i) >= 0) {
        lua_pushvalue(L, i);
        value.i64 = luaL_ref(L, LUA_REGISTRYINDEX) | 0x1'0000'0000;
        getstatewasmenv(L).trackExternRef(value.i64);
      }
      break;
    default:
      luaL_error(L, "unexpected wasm value type at language boundary. can only convert i32, i64, f32, f64, and externref.\n");
  }
}

struct PlutoFuncRef {
  soup::WasmSharedEnvironment::ScriptRaii script;
  uint32_t index;
};

static PlutoFuncRef *checkfuncref (lua_State *L, int i) {
  return (PlutoFuncRef*)luaL_checkudata(L, 1, "pluto:wasm-funcref");;
}

static int funcref_call (lua_State *L) {
  auto ptr = checkfuncref(L, 1);
  const auto& type = ptr->script->types[ptr->script->getTypeIndexForFunction(ptr->index)];
  int expected_top = 1 + static_cast<int>(type.parameters.size());  /* self + parameters */
  lua_checkstack(L, (expected_top - lua_gettop(L)) + 2);  /* parameters + 2 vectors */
  while (lua_gettop(L) != expected_top) {
    lua_pushnil(L);
  }
  auto& args = *pluto_newclassinst(L, std::vector<soup::WasmValue>);
  auto& out = *pluto_newclassinst(L, std::vector<soup::WasmValue>);
  for (uint32_t i = 0; i != type.parameters.size(); ++i) {
    opt_wasm_value(L, 2 + i, args.emplace_back());
  }
  const auto prev_callback_L = callback_L;
  callback_L = L;
  if (!ptr->script->call(ptr->index, std::move(args), &out)
    || out.size() < type.results.size() // Not using != because the stack may be over-filled as implicit drops only happen on internal calls.
    ) {
    callback_L = prev_callback_L;
    luaL_error(L, "wasm vm error");
  }
  callback_L = prev_callback_L;
  wasm_to_lua_stack(L, *ptr->script->shared_env, out, type.results);
  return static_cast<int>(type.results.size());
}

static void pushfuncref (lua_State *L, const soup::WasmSharedEnvironment::FuncRef& funcref) {
  auto ptr = new (lua_newuserdata(L, sizeof(PlutoFuncRef))) PlutoFuncRef{ *funcref.source, funcref.index };
  if (l_unlikely(luaL_newmetatable(L, "pluto:wasm-funcref"))) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State* L) {
      pluto_errorifnotgc(L);
      std::destroy_at<>(checkfuncref(L, 1));
      return 0;
    });
    lua_settable(L, -3);
    lua_pushliteral(L, "__call");
    lua_pushcfunction(L, &funcref_call);
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
}

static int call (lua_State *L) {
  auto& inst = checkmodule(L, 1);
  auto func_idx = inst.getExportedFuntion2(pluto_checkstring(L, 2));
  if (l_unlikely(func_idx >= inst.function_imports.size() + inst.functions.size())) {
    luaL_error(L, "function export not found");
  }
  const auto& type = inst.types[inst.getTypeIndexForFunction(func_idx)];
  int expected_top = 2 + static_cast<int>(type.parameters.size());  /* self + funcname + parameters */
  lua_checkstack(L, (expected_top - lua_gettop(L)) + 2);  /* parameters + 2 vectors */
  while (lua_gettop(L) != expected_top) {
    lua_pushnil(L);
  }
  auto& args = *pluto_newclassinst(L, std::vector<soup::WasmValue>);
  auto& out = *pluto_newclassinst(L, std::vector<soup::WasmValue>);
  for (uint32_t i = 0; i != type.parameters.size(); ++i) {
    opt_wasm_value(L, 3 + i, args.emplace_back(type.parameters[i]));
  }
  const auto prev_callback_L = callback_L;
  callback_L = L;
  if (!inst.call(func_idx, std::move(args), &out)
    || out.size() < type.results.size() // Not using != because the stack may be over-filled as implicit drops only happen on internal calls.
    ) {
    callback_L = prev_callback_L;
    luaL_error(L, "wasm vm error");
  }
  callback_L = prev_callback_L;
  wasm_to_lua_stack(L, *inst.shared_env, out, type.results);
  return static_cast<int>(type.results.size());
}

static int wasm_module_read (lua_State *L) {
  auto& inst = checkmodule(L, 1);
  lua_Unsigned addr = luaL_checkinteger(L, 2);
  lua_Unsigned size = luaL_checkinteger(L, 3);
  if (l_unlikely(!inst.memory)) {
    luaL_error(L, "module has no memory");
  }
  auto ptr = inst.memory->getView(addr, size);
  if (l_unlikely(!ptr)) {
    luaL_error(L, "attempt to read past end of memory");
  }
  lua_pushlstring(L, (const char*)ptr, size);
  return 1;
}

static int wasm_module_write (lua_State *L) {
  auto& inst = checkmodule(L, 1);
  lua_Unsigned addr = luaL_checkinteger(L, 2);
  size_t size;
  const char *data = luaL_checklstring(L, 3, &size);
  if (l_unlikely(!inst.memory)) {
    luaL_error(L, "module has no memory");
  }
  auto ptr = inst.memory->getView(addr, size);
  if (l_unlikely(!ptr)) {
    luaL_error(L, "attempt to write past end of memory");
  }
  memcpy(ptr, data, size);
  return 0;
}

static void call_imported_func (soup::WasmVm& vm, uint32_t user_data, const soup::WasmFunctionType& type) {
  lua_assert(callback_L);
  const auto L = callback_L;
  lua_rawgeti(L, LUA_REGISTRYINDEX, user_data);
  wasm_to_lua_stack(L, *vm.script.shared_env, vm.stack, type.parameters);
  lua_call(L, static_cast<int>(type.parameters.size()), static_cast<int>(type.results.size()));
  for (int i = 0; i != type.results.size(); ++i) {
    opt_wasm_value(L, (static_cast<int>(type.results.size()) - i) * -1, vm.stack.emplace_back(type.results[i]));
  }
}

static soup::WasmSharedEnvironment& getstatewasmenv (lua_State *L) {
  if (lua_getfield(L, LUA_REGISTRYINDEX, "pluto:statewasmenv") == LUA_TNIL) {
    lua_pop(L, 1);
    auto env = (soup::WasmSharedEnvironment*)pluto_setupgcmt(L, new (lua_newuserdata(L, sizeof(soup::WasmSharedEnvironment))) soup::WasmSharedEnvironment(), "soup::WasmSharedEnvironment", [](lua_State* L) {
      pluto_errorifnotgc(L);
      const auto prev_callback_L = callback_L;
      callback_L = L;
      std::destroy_at<>((soup::WasmSharedEnvironment*)luaL_checkudata(L, 1, "soup::WasmSharedEnvironment"));
      callback_L = prev_callback_L;
      return 0;
    });
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "pluto:statewasmenv");
    env->on_pre_free_script = [](soup::WasmScript& scr) {
      lua_assert(callback_L);
      for (const auto& imp : scr.function_imports) {
        if (imp.ptr == &call_imported_func) {
          luaL_unref(callback_L, LUA_REGISTRYINDEX, imp.user_data);
        }
      }
    };
    env->free_externref = [](uint64_t value) {
      lua_assert(callback_L);
	  luaL_unref(callback_L, LUA_REGISTRYINDEX, value & 0xffff'ffff);
    };
  }
  return *reinterpret_cast<soup::WasmSharedEnvironment*>(lua_touserdata(L, -1));
}

static soup::WasmScript& pushmodule (lua_State *L) {
  auto& env = getstatewasmenv(L);
  auto ptr = new (lua_newuserdata(L, sizeof(soup::WasmSharedEnvironment::ScriptRaii))) soup::WasmSharedEnvironment::ScriptRaii(env.createScript());
  if (l_unlikely(luaL_newmetatable(L, "pluto:wasm-module"))) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State* L) {
      pluto_errorifnotgc(L);
      const auto prev_callback_L = callback_L;
      callback_L = L;
      std::destroy_at<>(checkmoduleraw(L, 1));
      callback_L = prev_callback_L;
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
  return **ptr;
}

static int instantiate (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  if (lua_gettop(L) < 2) {
    lua_newtable(L);
  }
  soup::MemoryRefReader r(data, size);
  auto& inst = pushmodule(L);
  if (l_unlikely(!inst.load(r))) {
    luaL_error(L, "failed to load wasm module");
  }
  for (size_t i = 0; i != inst.function_imports.size(); ++i) {
    auto& imp = inst.function_imports[i];
    pluto_pushstring(L, imp.module_name);
    if (l_unlikely(lua_gettable(L, 2) <= LUA_TNIL)) {
      luaL_error(L, "missing import module \"%s\"", imp.module_name.c_str());
    }
    pluto_pushstring(L, imp.field_name);
    if (l_unlikely(lua_gettable(L, -2) <= LUA_TNIL)) {
      luaL_error(L, R"(missing import function "%s" in module "%s")", imp.field_name.c_str(), imp.module_name.c_str());
    }
    imp.ptr = &call_imported_func;
    imp.user_data = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
  }
  for (size_t i = 0; i != inst.global_imports.size(); ++i) {
    const auto& imp = inst.global_imports[i];
    pluto_pushstring(L, imp.module_name);
    if (l_unlikely(lua_gettable(L, 2) <= LUA_TNIL)) {
      luaL_error(L, "missing import module \"%s\"", imp.module_name.c_str());
    }
    pluto_pushstring(L, imp.field_name);
    if (l_unlikely(lua_gettable(L, -2) <= LUA_TNIL)) {
      luaL_error(L, R"(missing import global "%s" in module "%s")", imp.field_name.c_str(), imp.module_name.c_str());
    }
    auto& g = *(soup::SharedPtr<soup::WasmValue>*)luaL_checkudata(L, -1, "soup::SharedPtr<soup::WasmValue>");
    if (imp.type != g->type || imp.mut != g->mut) {
      luaL_error(L, R"(type mismatch for import global "%s" in module "%s")", imp.field_name.c_str(), imp.module_name.c_str());
    }
    inst.globals[i] = g;
    lua_pop(L, 2);
  }
  if (inst.memory_import) {
    const auto& imp = *inst.memory_import;
    pluto_pushstring(L, imp.module_name);
    if (l_unlikely(lua_gettable(L, 2) <= LUA_TNIL)) {
      luaL_error(L, "missing import module \"%s\"", imp.module_name.c_str());
    }
    pluto_pushstring(L, imp.field_name);
    if (l_unlikely(lua_gettable(L, -2) <= LUA_TNIL)) {
      luaL_error(L, R"(missing import global "%s" in module "%s")", imp.field_name.c_str(), imp.module_name.c_str());
    }
    auto& mem = *(soup::SharedPtr<soup::WasmScript::Memory>*)luaL_checkudata(L, -1, "soup::SharedPtr<soup::WasmScript::Memory>");
    if (!imp.isCompatibleWith(*mem)) {
      luaL_error(L, R"(type mismatch for import memory "%s" in module "%s")", imp.field_name.c_str(), imp.module_name.c_str());
    }
    inst.memory = mem;
    lua_pop(L, 2);
  }
  const auto prev_callback_L = callback_L;
  callback_L = L;
  if (l_unlikely(!inst.instantiate())) {
    getstatewasmenv(L).collectGarbage();
    callback_L = prev_callback_L;
    luaL_error(L, "failed to instantiate wasm module");
  }
  callback_L = prev_callback_L;
  return 1;
}

const char* ie_kind_names[] = { "function", "table", "memory", "global" };

static int describe (lua_State *L) {
  soup::WasmScript *inst;
  if (lua_type(L, 1) == LUA_TSTRING) {
    size_t size;
    const char *data = luaL_checklstring(L, 1, &size);
    inst = pluto_newclassinst(L, soup::WasmScript);
    soup::MemoryRefReader r(data, size);
    if (l_unlikely(!inst->load(r))) {
      luaL_error(L, "failed to load wasm module");
    }
  }
  else {
    inst = &checkmodule(L, 1);
  }
  lua_newtable(L);
  {
    lua_pushliteral(L, "imports");
    lua_newtable(L);
    lua_Integer i = 0;
    for (const auto& e : inst->function_imports) {
      lua_pushinteger(L, ++i);
      lua_newtable(L); {
        lua_pushliteral(L, "module");
        pluto_pushstring(L, e.module_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "name");
        pluto_pushstring(L, e.field_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "kind");
        lua_pushliteral(L, "function");
        lua_settable(L, -3);
      }
      lua_settable(L, -3);
    }
    for (const auto& e : inst->global_imports) {
      lua_pushinteger(L, ++i);
      lua_newtable(L); {
        lua_pushliteral(L, "module");
        pluto_pushstring(L, e.module_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "name");
        pluto_pushstring(L, e.field_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "kind");
        lua_pushliteral(L, "global");
        lua_settable(L, -3);
      }
      lua_settable(L, -3);
    }
    for (const auto& e : inst->table_imports) {
      lua_pushinteger(L, ++i);
      lua_newtable(L); {
        lua_pushliteral(L, "module");
        pluto_pushstring(L, e.module_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "name");
        pluto_pushstring(L, e.field_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "kind");
        lua_pushliteral(L, "table");
        lua_settable(L, -3);
      }
      lua_settable(L, -3);
    }
    if (inst->memory_import) {
      lua_pushinteger(L, ++i);
      lua_newtable(L); {
        lua_pushliteral(L, "module");
        pluto_pushstring(L, inst->memory_import->module_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "name");
        pluto_pushstring(L, inst->memory_import->field_name);
        lua_settable(L, -3);
        lua_pushliteral(L, "kind");
        lua_pushliteral(L, "table");
        lua_settable(L, -3);
      }
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
  {
    lua_pushliteral(L, "exports");
    lua_newtable(L);
    lua_Integer i = 0;
    for (const auto& e : inst->export_map) {
      if (l_likely(e.second.kind <= soup::WasmScript::IE_kGlobal)) {
        lua_pushinteger(L, ++i);
        lua_newtable(L); {
          lua_pushliteral(L, "name");
          pluto_pushstring(L, e.first);
          lua_settable(L, -3);
          lua_pushliteral(L, "kind");
          lua_pushstring(L, ie_kind_names[e.second.kind]);
          lua_settable(L, -3);
        }
        lua_settable(L, -3);
      }
    }
    lua_settable(L, -3);
  }
  return 1;
}

static const luaL_Reg funcs_wasm[] = {
  {"instantiate", instantiate},
  {"describe", describe},
  {nullptr, nullptr}
};

static int global_new (lua_State *L) {
  size_t len;
  const char *data = luaL_checklstring(L, 1, &len);
  bool mut = false;
  if (len >= 4 && memcmp(data, "mut ", 4) == 0) {
    mut = true;
    data += 4;
    len -= 4;
  }
  soup::WasmType type;
  if (len == 3 && memcmp(data, "i32", 3) == 0) {
    type = soup::WASM_I32;
  }
  else if (len == 3 && memcmp(data, "i64", 3) == 0) {
    type = soup::WASM_I64;
  }
  else if (len == 3 && memcmp(data, "f32", 3) == 0) {
    type = soup::WASM_F32;
  }
  else if (len == 3 && memcmp(data, "f64", 3) == 0) {
    type = soup::WASM_F64;
  }
  else if (len == 9 && memcmp(data, "externref", 9) == 0) {
    type = soup::WASM_EXTERNREF;
  }
  else {
    luaL_error(L, "invalid wasm type name (%s). expected i32, i64, f32, f64, externref.", data);
  }
  const bool init_value = lua_gettop(L) >= 2;
  auto& g = **pluto_newclassinst(L, soup::SharedPtr<soup::WasmValue>, soup::make_shared<soup::WasmValue>(type));
  g.mut = mut;
  if (init_value) {
    opt_wasm_value(L, 2, g);
  }
  return 1;
}

static const luaL_Reg funcs_wasm_global[] = {
  {"new", global_new},
  {nullptr, nullptr}
};

static int memory_new (lua_State* L) {
  const soup::wasm_uptr_t initial_pages = luaL_checkinteger(L, 1);
  const soup::wasm_uptr_t max_pages = luaL_optinteger(L, 2, 0x10'000);
  const bool _64bit = lua_istrue(L, 3);
  pluto_newclassinst(L, soup::SharedPtr<soup::WasmScript::Memory>, soup::make_shared<soup::WasmScript::Memory>(initial_pages, max_pages, _64bit));
  return 1;
}

static const luaL_Reg funcs_wasm_memory[] = {
  {"new", memory_new},
  {nullptr, nullptr}
};

int luaopen_wasm (lua_State *L) {
  luaL_newlib(L, funcs_wasm);

  lua_pushliteral(L, "global");
  luaL_newlib(L, funcs_wasm_global);
  lua_settable(L, -3);

  lua_pushliteral(L, "memory");
  luaL_newlib(L, funcs_wasm_memory);
  lua_settable(L, -3);

  return 1;
}

const Pluto::PreloadedLibrary Pluto::preloaded_wasm{ PLUTO_WASMLIBNAME, funcs_wasm, &luaopen_wasm};
