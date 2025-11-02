#define LUA_LIB
#include "lualib.h"

#include <cstring> // strcmp
#include <unordered_set>
#include <vector>

#include "ldo.h"

#include "vendor/Soup/soup/ffi.hpp"
#include "vendor/Soup/soup/Notifyable.hpp"
#include "vendor/Soup/soup/rflFunc.hpp"
#include "vendor/Soup/soup/rflParser.hpp"
#include "vendor/Soup/soup/rflStruct.hpp"
#include "vendor/Soup/soup/SharedLibrary.hpp"

static thread_local lua_State* callback_L = nullptr;

enum FfiType : uint8_t {
  FFI_UNKNOWN = 0,
  FFI_VOID,
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
  FFI_STR,
};

[[nodiscard]] static FfiType check_ffi_type (lua_State *L, int i) {
  const char *str = luaL_checkstring(L, i);
  if (strcmp(str, "void") == 0) return FFI_VOID;
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
  if (strcmp(str, "str") == 0) return FFI_STR;
  luaL_error(L, "unknown type '%s'", str);
}

[[nodiscard]] static FfiType rfl_type_to_ffi_type (const soup::rflType& type) noexcept {
  if (type.at == soup::rflType::DIRECT) {
    if (type.name == "void") { return FFI_VOID; }
    if (type.name == "bool") { return FFI_U8; }
    if (type.name == "char") { return FFI_I8; }
    if (type.name == "unsigned char") { return FFI_U8; }
    if (type.name == "int8_t") { return FFI_I8; }
    if (type.name == "uint8_t") { return FFI_U8; }
    if (type.name == "short") { return FFI_I16; }
    if (type.name == "unsigned short") { return FFI_U16; }
    if (type.name == "int16_t") { return FFI_I16; }
    if (type.name == "uint16_t") { return FFI_U16; }
    if (type.name == "int") { return FFI_I32; }
    if (type.name == "unsigned int") { return FFI_U32; }
    if (type.name == "int32_t") { return FFI_I32; }
    if (type.name == "uint32_t") { return FFI_U32; }
    if (type.name == "int64_t") { return FFI_I64; }
    if (type.name == "uint64_t") { return FFI_U64; }
    if (type.name == "long long") { return FFI_I64; }
    if (type.name == "unsigned long long") { return FFI_U64; }
    if (type.name == "size_t") { return SOUP_BITS == 64 ? FFI_U64 : FFI_U32; }
    if (type.name == "float") { return FFI_F32; }
    if (type.name == "double") { return FFI_F64; }
    return FFI_UNKNOWN;
  }
  else if (type.at == soup::rflType::POINTER) {
    if (type.name == "const char" || type.name == "char") {
      return FFI_STR;
    }
  }
  return FFI_PTR;
}

static int push_ffi_value (lua_State *L, FfiType type, const void *value) {
  switch (type) {
    case FFI_UNKNOWN:
      /* 'handling' this case makes the compiler happy */
      break;
    case FFI_VOID:
      return 0;
    case FFI_I8:
      lua_pushinteger(L, *reinterpret_cast<const int8_t*>(value));
      return 1;
    case FFI_I16:
      lua_pushinteger(L, *reinterpret_cast<const int16_t*>(value));
      return 1;
    case FFI_I32:
      lua_pushinteger(L, *reinterpret_cast<const int32_t*>(value));
      return 1;
    case FFI_I64:
      lua_pushinteger(L, *reinterpret_cast<const int64_t*>(value));
      return 1;
    case FFI_U8:
      lua_pushinteger(L, *reinterpret_cast<const uint8_t*>(value));
      return 1;
    case FFI_U16:
      lua_pushinteger(L, *reinterpret_cast<const uint16_t*>(value));
      return 1;
    case FFI_U32:
      lua_pushinteger(L, *reinterpret_cast<const uint32_t*>(value));
      return 1;
    case FFI_U64:
      lua_pushinteger(L, *reinterpret_cast<const uint64_t*>(value));
      return 1;
    case FFI_F32:
      lua_pushnumber(L, *reinterpret_cast<const float*>(value));
      return 1;
    case FFI_F64:
      lua_pushnumber(L, *reinterpret_cast<const double*>(value));
      return 1;
    case FFI_PTR:
      lua_pushlightuserdata(L, *reinterpret_cast<void* const*>(value));
      return 1;
    case FFI_STR:
      lua_pushstring(L, *reinterpret_cast<const char* const*>(value));
      return 1;
  }
  SOUP_UNREACHABLE;
}

static uint64_t check_ffi_value (lua_State *L, int i, FfiType type) {
  switch (type) {
    case FFI_UNKNOWN:
      /* 'handling' this case makes the compiler happy */
      break;
    case FFI_VOID:
      return 0;
    case FFI_I8:
      return static_cast<uint64_t>(static_cast<int8_t>(luaL_checkinteger(L, i)));
    case FFI_I16:
      return static_cast<uint64_t>(static_cast<int16_t>(luaL_checkinteger(L, i)));
    case FFI_I32:
      return static_cast<uint64_t>(static_cast<int32_t>(luaL_checkinteger(L, i)));
    case FFI_I64:
      return static_cast<uint64_t>(static_cast<int64_t>(luaL_checkinteger(L, i)));
    case FFI_U8:
      return static_cast<uint64_t>(static_cast<uint8_t>(luaL_checkinteger(L, i)));
    case FFI_U16:
      return static_cast<uint64_t>(static_cast<uint16_t>(luaL_checkinteger(L, i)));
    case FFI_U32:
      return static_cast<uint64_t>(static_cast<uint32_t>(luaL_checkinteger(L, i)));
    case FFI_U64:
      return static_cast<uint64_t>(luaL_checkinteger(L, i));
    case FFI_F32: {
      auto val = static_cast<float>(luaL_checknumber(L, i));
      return static_cast<uint64_t>(*reinterpret_cast<uint32_t*>(&val));
      static_assert(sizeof(float) == sizeof(uint32_t));
    }
    case FFI_F64: {
      auto val = static_cast<double>(luaL_checknumber(L, i));
      return *reinterpret_cast<uint64_t*>(&val);
      static_assert(sizeof(double) == sizeof(uint64_t));
    }
    case FFI_PTR:
      if (lua_type(L, i) == LUA_TTABLE) {
        lua_pushliteral(L, "__ffiaddr");
        if (lua_gettable(L, i) == LUA_TLIGHTUSERDATA) {
          const auto val = reinterpret_cast<uint64_t>(lua_touserdata(L, -1));
          lua_pop(L, 1);
          return val;
        }
        lua_pop(L, 1);
      }
      if (lua_type(L, i) != LUA_TUSERDATA)
        luaL_checktype(L, i, LUA_TLIGHTUSERDATA);
      return reinterpret_cast<uint64_t>(lua_touserdata(L, i));
    case FFI_STR:
      if (lua_type(L, i) == LUA_TNIL)
        return 0;
      return reinterpret_cast<uint64_t>(luaL_checkstring(L, i));
  }
  SOUP_UNREACHABLE;
}

static void *weaklytestudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      lua_pushliteral(L, "__name");
      if (lua_gettable(L, -2) != LUA_TSTRING || strcmp(lua_tostring(L, -1), tname) != 0) {
        p = NULL;  /* value is a userdata with wrongly-named metatable */
      }
      lua_pop(L, 2);  /* remove metatable and __name */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}


static void *weaklycheckudata (lua_State *L, int ud, const char *tname) {
  void *p = weaklytestudata(L, ud, tname);
  luaL_argexpected(L, p != NULL, ud, tname);
  return p;
}

struct FfiFuncWrapper {
  void* addr;
  std::vector<FfiType> args;
  FfiType ret;
  soup::SharedPtr<soup::SharedLibrary> owner;
};

[[nodiscard]] static soup::SharedPtr<soup::SharedLibrary>* checkffilib (lua_State *L, int i) {
  return (soup::SharedPtr<soup::SharedLibrary>*)luaL_checkudata(L, i, "pluto:ffi-library");
}

[[nodiscard]] static soup::SharedPtr<soup::SharedLibrary>* checkffilibfromtable (lua_State *L, int i) {
  lua_pushliteral(L, "__object");
  if (!lua_gettable(L, i)) {
    luaL_typeerror(L, i, "pluto:ffi-library");
  }
  auto lib = checkffilib(L, -1);
  lua_pop(L, 1);
  return lib;
}

[[nodiscard]] static FfiFuncWrapper* checkfuncwrapper (lua_State *L, int i) {
  return (FfiFuncWrapper*)luaL_checkudata(L, i, "pluto:ffi-funcwrapper");
}

#ifdef PLUTO_FFI_CALL_HOOK
extern bool PLUTO_FFI_CALL_HOOK (lua_State *L, void *addr);
#endif

static int ffi_funcwrapper_call (lua_State *L) {
  auto fw = checkfuncwrapper(L, lua_upvalueindex(1));
#ifdef PLUTO_FFI_CALL_HOOK
  if (!PLUTO_FFI_CALL_HOOK(L, fw->addr)) {
    luaL_error(L, "disallowed by content moderation policy");
  }
#endif
  uintptr_t args[soup::ffi::MAX_ARGS];
  int i = 0;
  for (const auto& arg_type : fw->args) {
    lua_assert(i < soup::ffi::MAX_ARGS);
    args[i] = check_ffi_value(L, 1 + i, arg_type);
    ++i;
  }
  uintptr_t retval;
  callback_L = L;
  try {
    retval = soup::ffi::call(fw->addr, args, i);
  }
  catch (const std::exception& e) {
    callback_L = nullptr;
    luaL_error(L, "C++ exception: %s", e.what());
  }
  catch (...) {
    callback_L = nullptr;
    luaL_error(L, "C++ exception");
  }
  callback_L = nullptr;
  return push_ffi_value(L, fw->ret, &retval);
}

static FfiFuncWrapper *newfuncwrapper (lua_State *L) {
  auto fw = new (lua_newuserdata(L, sizeof(FfiFuncWrapper))) FfiFuncWrapper();
  if (luaL_newmetatable(L, "pluto:ffi-funcwrapper")) {
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkfuncwrapper(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  lua_pushcclosure(L, ffi_funcwrapper_call, 1);
  return fw;
}

static int ffi_lib_wrap (lua_State *L) {
  auto fw = newfuncwrapper(L);
  fw->ret = check_ffi_type(L, 2);
  const char *name = luaL_checkstring(L, 3);
  auto pSpLib = checkffilibfromtable(L, 1);
  fw->addr = (*pSpLib)->getAddress(name);
  if (l_unlikely(fw->addr == nullptr)) {
    luaL_error(L, "could not find '%s' in library", name);
  }
  const auto nargtypes = lua_gettop(L) - 4;
  if (nargtypes > soup::ffi::MAX_ARGS) {
    luaL_error(L, "function has too many parameters");
  }
  fw->args.reserve(nargtypes);
  for (int i = 4; i != 4 + nargtypes; ++i) {
    fw->args.emplace_back(check_ffi_type(L, i));
  }
  fw->owner = *pSpLib;
  return 1;
}

static int ffi_lib_value (lua_State *L) {
  const char *name = luaL_checkstring(L, 3);
  void *addr = checkffilibfromtable(L, 1)->get()->getAddress(luaL_checkstring(L, 3));
  if (l_unlikely(addr == nullptr)) {
    luaL_error(L, "could not find '%s' in library", name);
  }
  return push_ffi_value(L, check_ffi_type(L, 2), addr);
}

static int ffi_lib_cdef (lua_State *L) {
  const auto pSpLib = checkffilibfromtable(L, 1);
  const auto lib = pSpLib->get();
  auto par = pluto_newclassinst(L, soup::rflParser, pluto_checkstring(L, 2));
  auto var = pluto_newclassinst(L, soup::rflVar);
  auto func = pluto_newclassinst(L, soup::rflFunc);
  bool fail = false;
  while (par->hasMore()) {
    const auto i = par->i;
    try {
      *var = par->readVar();
    }
    catch (...) {
      fail = true;
    }
    if (l_unlikely(fail))
      luaL_error(L, "malformed variable");
    if (par->align(), par->i->isLiteral() && par->i->val.getString() == "(") {
      par->i = i;
      try {
          *func = par->readFunc();
      }
      catch (...) {
        fail = false;
      }
      if (l_unlikely(fail))
        luaL_error(L, "malformed function");
      pluto_pushstring(L, func->name);
      auto fw = newfuncwrapper(L);
      fw->ret = rfl_type_to_ffi_type(func->return_type);
      if (l_unlikely(fw->ret == FFI_UNKNOWN))
        luaL_error(L, "malformed function");
      fw->addr = lib->getAddress(func->name.c_str());
      if (l_unlikely(fw->addr == nullptr)) {
        luaL_error(L, "could not find '%s' in library", func->name.c_str());
      }
      if (func->parameters.size() > soup::ffi::MAX_ARGS) {
        luaL_error(L, "'%s' has too many parameters", func->name.c_str());
      }
      fw->args.reserve(func->parameters.size());
      for (const auto& param : func->parameters) {
        fw->args.emplace_back(rfl_type_to_ffi_type(param.type));
        if (l_unlikely(fw->args.back() == FFI_UNKNOWN))
          luaL_error(L, "malformed function");
      }
      fw->owner = *pSpLib;
      lua_settable(L, 1);
    }
    else {
      void *addr = lib->getAddress(var->name.c_str());
      if (!addr) {
        luaL_error(L, "could not find '%s' in library", var->name.c_str());
      }
      pluto_pushstring(L, var->name);
      const auto type = rfl_type_to_ffi_type(var->type);
      if (l_unlikely(type == FFI_UNKNOWN)) {
        luaL_error(L, "unknown type name");
      }
      push_ffi_value(L, type, addr);
      lua_settable(L, 1);
    }
  }
  return 0;
}

static int ffi_open (lua_State *L) {
#ifndef PLUTO_NO_BINARIES
  const char *libname = luaL_checkstring(L, 1);
  lua_newtable(L);
  lua_pushliteral(L, "__object");
  {
    auto spLib = soup::make_shared<soup::SharedLibrary>(libname);
    auto lib = new (lua_newuserdata(L, sizeof(soup::SharedPtr<soup::SharedLibrary>))) soup::SharedPtr<soup::SharedLibrary>(std::move(spLib));
    if (luaL_newmetatable(L, "pluto:ffi-library")) {
      lua_pushliteral(L, "__gc");
      lua_pushcfunction(L, [](lua_State *L) {
        std::destroy_at<>(checkffilib(L, 1));
        return 0;
      });
      lua_settable(L, -3);
    }
    lua_setmetatable(L, -2);
    if (!(*lib)->isLoaded()) {
      luaL_error(L, "failed to load library '%s'", libname);
    }
  }
  lua_settable(L, -3);
  lua_pushliteral(L, "wrap");
  lua_pushcfunction(L, ffi_lib_wrap);
  lua_settable(L, -3);
  lua_pushliteral(L, "value");
  lua_pushcfunction(L, ffi_lib_value);
  lua_settable(L, -3);
  lua_pushliteral(L, "cdef");
  lua_pushcfunction(L, ffi_lib_cdef);
  lua_settable(L, -3);
#else
  PLUTO_NO_BINARIES_FAIL
#endif
  return 1;
}

struct FfiStruct : public soup::rflStruct {
  inline auto operator=(soup::rflStruct&& strct) noexcept {
    return soup::rflStruct::operator=(std::move(strct));
  }

  [[nodiscard]] const soup::rflType& getMemberType(const std::string& memname) const {
    for (const auto& mem : this->members) {
      if (mem.name == memname) {
        return mem.type;
      }
    }
    SOUP_ASSERT_UNREACHABLE;
  }
};

static int ffi_push_new (lua_State *L, int i) {
  const auto strct = (FfiStruct*)weaklycheckudata(L, i, "pluto:ffi-struct-type");
  const auto size = strct->getSize();
  void* area = lua_newuserdata(L, size + sizeof(uintptr_t) - 1);  /* extra space for __newindex */
  memset(area, 0, size);  /* probably best for memory we give to a script... */
  lua_newtable(L);
  lua_pushliteral(L, "type");
  lua_pushvalue(L, i == -1 ? -4 : i);
  lua_settable(L, -3);
  lua_pushliteral(L, "__index");
  lua_pushcfunction(L, [](lua_State *L) {
    void *udata = lua_touserdata(L, 1);
    if (lua_getmetatable(L, 1)) {
      lua_pushliteral(L, "type");
      lua_gettable(L, -2);
      const auto strct = (FfiStruct*)weaklycheckudata(L, -1, "pluto:ffi-struct-type");
      const auto memname = pluto_checkstring(L, 2);
      const auto offset = strct->getOffsetOf(memname);
      if (l_unlikely(offset == -1)) {
        luaL_error(L, "no member with name '%s'", memname.c_str());
      }
      return push_ffi_value(L, rfl_type_to_ffi_type(strct->getMemberType(memname)), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(udata) + offset));
    }
    return 0;
  });
  lua_settable(L, -3);
  lua_pushliteral(L, "__newindex");
  lua_pushcfunction(L, [](lua_State *L) {
    void *udata = lua_touserdata(L, 1);
    if (lua_getmetatable(L, 1)) {
      lua_pushliteral(L, "type");
      lua_gettable(L, -2);
      const auto strct = (FfiStruct*)weaklycheckudata(L, -1, "pluto:ffi-struct-type");
      const auto memname = pluto_checkstring(L, 2);
      const auto offset = strct->getOffsetOf(memname);
      if (l_unlikely(offset == -1)) {
        luaL_error(L, "no member with name '%s'", memname.c_str());
      }
      const soup::rflType& type = strct->getMemberType(memname);
      uint64_t new_data = check_ffi_value(L, 3, rfl_type_to_ffi_type(type));
      memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(udata) + offset), &new_data, type.getSize());
    }
    return 0;
  });
  lua_settable(L, -3);
  lua_setmetatable(L, -2);
  return 1;
}

static FfiStruct *ffi_new_struct_type (lua_State *L) {
  auto strct = new (lua_newuserdata(L, sizeof(FfiStruct))) FfiStruct();
  lua_newtable(L);
  lua_pushliteral(L, "__name");
  lua_pushliteral(L, "pluto:ffi-struct-type");
  lua_settable(L, -3);
  lua_pushliteral(L, "__gc");
  lua_pushcfunction(L, [](lua_State *L) {
    std::destroy_at<>((FfiStruct*)lua_touserdata(L, -1));
    return 0;
  });
  lua_settable(L, -3);
  lua_pushliteral(L, "__index");
  lua_newtable(L);  {
    lua_pushliteral(L, "new");
    lua_pushvalue(L, -5);
    lua_pushcclosure(L, [](lua_State *L) {
      return ffi_push_new(L, lua_upvalueindex(1));
    }, 1);
    lua_settable(L, -3);
  }
  lua_settable(L, -3);
  lua_setmetatable(L, -2);
  return strct;
}

static void validate_struct (lua_State *L, const FfiStruct& strct) {
  auto seen_before = pluto_newclassinst(L, std::unordered_set<std::string>);
  for (const auto& mem : strct.members) {
    if (l_unlikely(seen_before->count(mem.name) != 0)) {
      luaL_error(L, "duplicate member name '%s'", mem.name.c_str());
    }
    seen_before->emplace(mem.name);

    if (l_unlikely(rfl_type_to_ffi_type(mem.type) == FFI_UNKNOWN)) {
      luaL_error(L, "member '%s' has an unknown type", mem.name.c_str());
    }
  }
  lua_pop(L, 1);
}

static int ffi_struct (lua_State *L) {
  auto par = pluto_newclassinst(L, soup::rflParser, pluto_checkstring(L, 1));
  auto strct = ffi_new_struct_type(L);
  bool fail = false;
  try {
    *strct = par->readStruct();
  }
  catch (...) {
    fail = true;
  }
  if (l_unlikely(fail))
    luaL_error(L, "malformed struct");
  validate_struct(L, *strct);
  return 1;
}

static int ffi_new (lua_State *L) {
  if (lua_type(L, 1) == LUA_TSTRING) {
    lua_pushvalue(L, 1);
    if (lua_gettable(L, lua_upvalueindex(1)) > LUA_TNIL) {
      return ffi_push_new(L, -1);
    }
  }
  return ffi_push_new(L, 1);
}

static int ffi_cdef (lua_State *L) {
  auto par = pluto_newclassinst(L, soup::rflParser, pluto_checkstring(L, 1));
  lua_pushvalue(L, lua_upvalueindex(1));
  /* stack now: par, ffi */
  while (par->hasMore()) {
    auto strct = ffi_new_struct_type(L);
    bool fail = false;
    try {
      *strct = par->readStruct();
    }
    catch (...) {
      fail = true;
    }
    if (l_unlikely(fail))
      luaL_error(L, "malformed struct");
    luaL_check(L, strct->name.empty(), "anonymous structs not supported in ffi.cdef");
    validate_struct(L, *strct);
    /* stack now: par, ffi, strct */
    pluto_pushstring(L, strct->name);
    /* stack now: par, ffi, strct, name */
    lua_pushvalue(L, -1);
    /* stack now: par, ffi, strct, name, name */
    if (lua_gettable(L, -4) > LUA_TNIL) {
      luaL_error(L, "multiple definitions of '%s'", strct->name.c_str());
    }
    /* stack now: par, ffi, strct, name, ffi.name */
    lua_pop(L, 1);
    /* stack now: par, ffi, strct, name */
    lua_pushvalue(L, -2);
    /* stack now: par, ffi, strct, name, strct */
    lua_settable(L, -4);
    /* stack now: par, ffi, strct */
    lua_pop(L, 1);
    /* stack now: par, ffi */
  }
  return 0;
}

static FfiStruct *check_struct_type (lua_State *L) {
  if (lua_type(L, 1) == LUA_TSTRING) {
    lua_pushvalue(L, 1);
    if (lua_gettable(L, lua_upvalueindex(1)) > LUA_TNIL) {
      return (FfiStruct*)weaklycheckudata(L, -1, "pluto:ffi-struct-type");
    }
  }
  if (void *addr = weaklytestudata(L, 1, "pluto:ffi-struct-type")) {
    return (FfiStruct*)addr;
  }
  luaL_checktype(L, 1, LUA_TUSERDATA);
  lua_getmetatable(L, 1);
  lua_pushliteral(L, "type");
  lua_gettable(L, -2);
  return (FfiStruct*)weaklycheckudata(L, -1, "pluto:ffi-struct-type");
}

static int ffi_sizeof (lua_State *L) {
  lua_pushinteger(L, check_struct_type(L)->getSize());
  return 1;
}

static int ffi_offsetof (lua_State *L) {
  const auto strct = check_struct_type(L);
  const auto memname = pluto_checkstring(L, 2);
  const auto offset = strct->getOffsetOf(memname);
  if (l_unlikely(offset == -1)) {
    luaL_error(L, "no member with name '%s'", memname.c_str());
  }
  lua_pushinteger(L, offset);
  return 1;
}

static int ffi_alloc (lua_State *L) {
  const auto size = luaL_checkinteger(L, 1);
  auto ud = lua_newuserdata(L, size);
  memset(ud, 0, size);
  return 1;
}

static int ffi_write (lua_State *L) {
  luaL_checktype(L, 1, LUA_TUSERDATA);
  auto dst_len = lua_rawlen(L, 1);
  auto dst = lua_touserdata(L, 1);
  size_t src_len;
  auto src = luaL_checklstring(L, 2, &src_len);
  if (src_len > dst_len) {
    src_len = dst_len;
  }
  memcpy(dst, src, src_len);
  return 0;
}

static int ffi_read (lua_State *L) {
  luaL_checktype(L, 1, LUA_TUSERDATA);
  auto size = lua_rawlen(L, 1);
  auto data = lua_touserdata(L, 1);
  lua_pushlstring(L, static_cast<char*>(data), size);
  return 1;
}

#if SOUP_FFI_CALLBACK_AVAILABLE
struct FfiCallback {
  void* trampoline = nullptr;
  uintptr_t defaultreturn = 0;
  lua_Number callcount = 0;
  std::vector<FfiType> args;
  FfiType ret;
  bool blocking = false;
  std::atomic<uint16_t> waiting = false;
  lua_State* L = nullptr;
  soup::Notifyable L_notify;
  soup::RecursiveMutex L_mtx;

  ~FfiCallback() {
    if (trampoline) {
      soup::ffi::callbackFree(trampoline);
    }
  }

  struct ExecData {
    const FfiCallback* cb;
    const uintptr_t* args;
    uintptr_t* retval;
  };

  void protectedExec(lua_State *L, const uintptr_t* args, uintptr_t& retval) const {
    lua_pushinteger(L, reinterpret_cast<uintptr_t>(this));
    if (lua_gettable(L, LUA_REGISTRYINDEX) == LUA_TFUNCTION) {
      for (size_t i = 0; i != this->args.size(); ++i) {
        push_ffi_value(L, this->args[i], &args[i]);
      }
      lua_call(L, (int)this->args.size(), ret != FFI_VOID);
      if (ret != FFI_VOID) {
        retval = check_ffi_value(L, -1, ret);
        lua_pop(L, 1);
      }
    }
  }

  static void staticProtectedExec(lua_State *L, void *ud) {
    auto& ed = *(ExecData*)ud;
    return ed.cb->protectedExec(L, ed.args, *ed.retval);
  }

  void exec(lua_State *L, const uintptr_t* args, uintptr_t& retval) const {
    ExecData ed{ this, args, &retval };
    if (luaD_pcall(L, staticProtectedExec, &ed, savestack(L, L->top.p), L->errfunc) != LUA_OK) {
      throw std::runtime_error(lua_tostring(L, -1));
    }
  }
};

static uintptr_t ffi_callback_trampoline (uintptr_t user_data, const uintptr_t* args) {
  auto& cb = *reinterpret_cast<FfiCallback*>(user_data);
  uintptr_t retval = cb.defaultreturn;
  ++cb.callcount;
  if (callback_L) {
    cb.exec(callback_L, args, retval);
  }
  else if (cb.blocking) {
    ++cb.waiting;
    while (true) {
      if (!cb.L)
        cb.L_notify.wait();
      cb.L_mtx.lock();
      if (cb.L) {
        --cb.waiting;
        cb.exec(cb.L, args, retval);
        cb.L_mtx.unlock();
        break;
      }
      cb.L_mtx.unlock();
    }
  }
  return retval;
}

static int ffi_callback (lua_State *L) {
  const auto nargs = lua_gettop(L) - 2;
  if (nargs < 0)
    luaL_error(L, "expected at least 2 arguments");
  if (nargs > soup::ffi::MAX_ARGS)
    luaL_error(L, "callback has too many parameters");
  luaL_checktype(L, nargs + 2, LUA_TFUNCTION);

  auto& cb = *pluto_newclassinst(L, FfiCallback);
  cb.ret = check_ffi_type(L, 1);
  cb.args.reserve(nargs);
  for (int i = 0; i != nargs; ++i) {
    cb.args.emplace_back(check_ffi_type(L, 2 + i));
  }
  cb.trampoline = soup::ffi::callbackAlloc(ffi_callback_trampoline, reinterpret_cast<uintptr_t>(&cb));
  if (!cb.trampoline) {
#if SOUP_APPLE
    luaL_error(L, "Failed to allocate an FFI callback. Is the 'com.apple.security.cs.allow-jit' entitlement set?");
#else
    luaL_error(L, "Failed to allocate an FFI callback.");
#endif
  }
  lua_pushinteger(L, reinterpret_cast<uintptr_t>(&cb) + 1);
  lua_pushvalue(L, -2);
  lua_settable(L, LUA_REGISTRYINDEX);

  lua_pushinteger(L, reinterpret_cast<uintptr_t>(&cb));
  lua_pushvalue(L, nargs + 2);
  lua_settable(L, LUA_REGISTRYINDEX);

  lua_newtable(L);
  lua_pushliteral(L, "__ffiaddr");
  lua_pushlightuserdata(L, cb.trampoline);
  lua_settable(L, -3);
  lua_pushliteral(L, "__object");
  lua_pushvalue(L, -3);
  lua_settable(L, -3);
  lua_pushliteral(L, "blocking");
  lua_pushcfunction(L, [](lua_State *L) -> int {
    const auto nargs = lua_gettop(L);
    lua_pushliteral(L, "__object");
    if (lua_gettable(L, 1) == LUA_TUSERDATA) {
      auto& cb = *(FfiCallback*)lua_touserdata(L, -1);
      if (nargs == 1) {
        lua_pushboolean(L, cb.blocking);
        return 1;
      }
      cb.blocking = lua_toboolean(L, 2);
    }
    return 0;
  });
  lua_settable(L, -3);
  lua_pushliteral(L, "defaultreturn");
  lua_pushcfunction(L, [](lua_State *L) -> int {
    const auto nargs = lua_gettop(L);
    lua_pushliteral(L, "__object");
    if (lua_gettable(L, 1) == LUA_TUSERDATA) {
      auto& cb = *(FfiCallback*)lua_touserdata(L, -1);
      if (nargs == 1) {
        push_ffi_value(L, cb.ret, &cb.defaultreturn);
        return 1;
      }
      cb.defaultreturn = check_ffi_value(L, 2, cb.ret);
    }
    return 0;
  });
  lua_settable(L, -3);
  lua_pushliteral(L, "callcount");
  lua_pushcfunction(L, [](lua_State *L) -> int {
    const auto nargs = lua_gettop(L);
    lua_pushliteral(L, "__object");
    if (lua_gettable(L, 1) == LUA_TUSERDATA) {
      auto& cb = *(FfiCallback*)lua_touserdata(L, -1);
      lua_pushnumber(L, cb.callcount);
      return 1;
    }
    return 0;
  });
  lua_settable(L, -3);
  lua_pushliteral(L, "tick");
  lua_pushcfunction(L, [](lua_State *L) -> int {
    lua_pushliteral(L, "__object");
    if (lua_gettable(L, 1) == LUA_TUSERDATA) {
      auto& cb = *(FfiCallback*)lua_touserdata(L, -1);
      cb.L_mtx.lock();
      cb.L = L;
      cb.L_mtx.unlock();
      while (cb.waiting.load()) {
        cb.L_notify.wakeOne();
        cb.L_mtx.lock();
        cb.L_mtx.unlock();
      }
      cb.L_mtx.lock();
      cb.L = nullptr;
      cb.L_mtx.unlock();
    }
    return 0;
  });
  lua_settable(L, -3);
  return 1;
}
#endif

static const luaL_Reg funcs_ffi[] = {
  {"open", ffi_open},
  {"struct", ffi_struct},
  {"alloc", ffi_alloc},
  {"write", ffi_write},
  {"read", ffi_read},
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_ffi(lua_State *L) {
  luaL_newlib(L, funcs_ffi);

#if SOUP_FFI_CALLBACK_AVAILABLE
  lua_pushliteral(L, "callback");
  lua_pushcfunction(L, ffi_callback);
  lua_settable(L, -3);
#endif

  lua_pushliteral(L, "new");
  lua_pushvalue(L, -2);
  lua_pushcclosure(L, ffi_new, 1);
  lua_settable(L, -3);

  lua_pushliteral(L, "cdef");
  lua_pushvalue(L, -2);
  lua_pushcclosure(L, ffi_cdef, 1);
  lua_settable(L, -3);

  lua_pushliteral(L, "sizeof");
  lua_pushvalue(L, -2);
  lua_pushcclosure(L, ffi_sizeof, 1);
  lua_settable(L, -3);

  lua_pushliteral(L, "offsetof");
  lua_pushvalue(L, -2);
  lua_pushcclosure(L, ffi_offsetof, 1);
  lua_settable(L, -3);

  lua_pushliteral(L, "nullptr");
  lua_pushlightuserdata(L, nullptr);
  lua_settable(L, -3);

  return 1;
}

const Pluto::PreloadedLibrary Pluto::preloaded_ffi{ PLUTO_FFILIBNAME, funcs_ffi, &luaopen_ffi};
