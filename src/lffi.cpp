#define LUA_LIB
#include "lualib.h"

#include <cstring> // strcmp
#include <vector>

#include "vendor/Soup/soup/ffi.hpp"
#include "vendor/Soup/soup/SharedLibrary.hpp"

enum FfiType : uint8_t {
  FFI_VOID = 0,
  FFI_I8,
  FFI_I16,
  FFI_I32,
  FFI_I64,
  FFI_U8,
  FFI_U16,
  FFI_U32,
  FFI_U64,
  FFI_F32,
  FFI_F64,
  FFI_PTR,
};

[[nodiscard]] static FfiType check_ffi_type (lua_State *L, int i) {
  const char *str = luaL_checkstring(L, i);
  if (strcmp(str, "i8") == 0) return FFI_I8;
  if (strcmp(str, "i16") == 0) return FFI_I16;
  if (strcmp(str, "i32") == 0) return FFI_I32;
  if (strcmp(str, "i64") == 0) return FFI_I64;
  if (strcmp(str, "u8") == 0) return FFI_U8;
  if (strcmp(str, "u16") == 0) return FFI_U16;
  if (strcmp(str, "u32") == 0) return FFI_U32;
  if (strcmp(str, "u64") == 0) return FFI_U64;
  if (strcmp(str, "f32") == 0) return FFI_F32;
  if (strcmp(str, "f64") == 0) return FFI_F64;
  if (strcmp(str, "ptr") == 0) return FFI_PTR;
  return FFI_VOID;
}

static int push_ffi_value (lua_State *L, FfiType type, void *value) {
  switch (type) {
    case FFI_VOID:
      return 0;
    case FFI_I8:
      lua_pushinteger(L, *reinterpret_cast<int8_t*>(value));
      return 1;
    case FFI_I16:
      lua_pushinteger(L, *reinterpret_cast<int16_t*>(value));
      return 1;
    case FFI_I32:
      lua_pushinteger(L, *reinterpret_cast<int32_t*>(value));
      return 1;
    case FFI_I64:
      lua_pushinteger(L, *reinterpret_cast<int64_t*>(value));
      return 1;
    case FFI_U8:
      lua_pushinteger(L, *reinterpret_cast<uint8_t*>(value));
      return 1;
    case FFI_U16:
      lua_pushinteger(L, *reinterpret_cast<uint16_t*>(value));
      return 1;
    case FFI_U32:
      lua_pushinteger(L, *reinterpret_cast<uint32_t*>(value));
      return 1;
    case FFI_U64:
      lua_pushinteger(L, *reinterpret_cast<uint64_t*>(value));
      return 1;
    case FFI_F32:
      lua_pushnumber(L, *reinterpret_cast<float*>(value));
      return 1;
    case FFI_F64:
      lua_pushnumber(L, *reinterpret_cast<double*>(value));
      return 1;
    case FFI_PTR:
      lua_pushinteger(L, *reinterpret_cast<uintptr_t*>(value));
      return 1;
  }
  SOUP_UNREACHABLE;
}

static uintptr_t check_ffi_value (lua_State *L, int i, FfiType type) {
  switch (type) {
    case FFI_VOID:
      return 0;
    case FFI_I8:
      return static_cast<uintptr_t>(static_cast<int8_t>(luaL_checkinteger(L, i)));
    case FFI_I16:
      return static_cast<uintptr_t>(static_cast<int16_t>(luaL_checkinteger(L, i)));
    case FFI_I32:
      return static_cast<uintptr_t>(static_cast<int32_t>(luaL_checkinteger(L, i)));
    case FFI_I64:
      return static_cast<uintptr_t>(static_cast<int64_t>(luaL_checkinteger(L, i)));
    case FFI_U8:
      return static_cast<uintptr_t>(static_cast<uint8_t>(luaL_checkinteger(L, i)));
    case FFI_U16:
      return static_cast<uintptr_t>(static_cast<uint16_t>(luaL_checkinteger(L, i)));
    case FFI_U32:
      return static_cast<uintptr_t>(static_cast<uint32_t>(luaL_checkinteger(L, i)));
    case FFI_U64:
      return static_cast<uintptr_t>(static_cast<uint64_t>(luaL_checkinteger(L, i)));
    case FFI_F32:
      return static_cast<uintptr_t>(static_cast<float>(luaL_checknumber(L, i)));
    case FFI_F64:
      return static_cast<uintptr_t>(static_cast<double>(luaL_checknumber(L, i)));
    case FFI_PTR:
      if (lua_type(L, i) == LUA_TSTRING) {
        return reinterpret_cast<uintptr_t>(luaL_checkstring(L, i));
      }
      return static_cast<uintptr_t>(luaL_checkinteger(L, i));
  }
  SOUP_UNREACHABLE;
}

struct FfiFuncWrapper {
  void* addr;
  std::vector<FfiType> args;
  FfiType ret;
};

[[nodiscard]] static soup::SharedLibrary* checksharedlibrary (lua_State *L, int i) {
  return (soup::SharedLibrary*)luaL_checkudata(L, i, "pluto:ffi.library");
}

[[nodiscard]] static FfiFuncWrapper* checkfuncwrapper (lua_State *L, int i) {
  return (FfiFuncWrapper*)luaL_checkudata(L, i, "pluto:ffi.funcwrapper");
}

#ifdef PLUTO_FFI_CALL_HOOK
extern bool PLUTO_FFI_CALL_HOOK (lua_State *L, void *addr);
#endif

static int ffi_funcwrapper_call (lua_State *L) {
  auto fw = checkfuncwrapper(L, 1);
#ifdef PLUTO_FFI_CALL_HOOK
  if (!PLUTO_FFI_CALL_HOOK(L, fw->addr)) {
    luaL_error(L, "disallowed by content moderation policy");
  }
#endif
  uintptr_t args[soup::ffi::MAX_ARGS];
  int i = 0;
  for (const auto& arg_type : fw->args) {
    lua_assert(i < soup::ffi::MAX_ARGS);
    args[i] = check_ffi_value(L, 2 + i, arg_type);
    ++i;
  }
  uintptr_t retval = soup::ffi::call(fw->addr, args, i);
  return push_ffi_value(L, fw->ret, &retval);
}

static int ffi_lib_wrap (lua_State *L) {
  auto fw = new (lua_newuserdata(L, sizeof(FfiFuncWrapper))) FfiFuncWrapper();
  if (luaL_newmetatable(L, "pluto:ffi.funcwrapper")) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkfuncwrapper(L, 1));
      return 0;
    });
    lua_settable(L, -3);
    lua_pushliteral(L, "__call");
    lua_pushcfunction(L, ffi_funcwrapper_call);
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  fw->ret = check_ffi_type(L, 2);
  fw->addr = checksharedlibrary(L, 1)->getAddress(luaL_checkstring(L, 3));
  const auto nargtypes = lua_gettop(L) - 4;
  if (nargtypes > soup::ffi::MAX_ARGS) {
    luaL_error(L, "too many arguments");
  }
  fw->args.reserve(nargtypes);
  for (int i = 4; i != 4 + nargtypes; ++i) {
    fw->args.emplace_back(check_ffi_type(L, i));
  }
  return 1;
}

static int ffi_lib_value (lua_State *L) {
  if (void* addr = checksharedlibrary(L, 1)->getAddress(luaL_checkstring(L, 3))) {
    return push_ffi_value(L, check_ffi_type(L, 2), addr);
  }
  return 0;
}

static int ffi_open (lua_State *L) {
#ifndef PLUTO_NO_BINARIES
  const char *libname = luaL_checkstring(L, 1);
  auto lib = new (lua_newuserdata(L, sizeof(soup::SharedLibrary))) soup::SharedLibrary(libname);
  if (luaL_newmetatable(L, "pluto:ffi.library")) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checksharedlibrary(L, 1));
      return 0;
    });
    lua_settable(L, -3);
    lua_pushliteral(L, "__index");
    lua_newtable(L); {
      lua_pushliteral(L, "wrap");
      lua_pushcfunction(L, ffi_lib_wrap);
      lua_settable(L, -3);
      lua_pushliteral(L, "value");
      lua_pushcfunction(L, ffi_lib_value);
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  if (!lib->isLoaded()) {
    luaL_error(L, "failed to load library '%s'", libname);
  }
#else
  PLUTO_NO_BINARIES_FAIL
#endif
  return 1;
}

static const luaL_Reg funcs_ffi[] = {
  {"open", ffi_open},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(ffi);
