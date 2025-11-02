#define LUA_LIB
#include "lualib.h"

#include <cstdlib> // malloc, realloc, free
#include <memory> // destroy_at
#include <new> // bad_alloc

#include "vendor/Soup/soup/Buffer.hpp"

#include "ldo.h"
#include "lmem.h"

#ifdef PLUTO_MEMORY_LIMIT
struct PlutoSingleBlockAllocator : public soup::memAllocator
{
    lua_State* L;
    size_t size = 0;

    PlutoSingleBlockAllocator()
        : memAllocator(&allocateImpl, &reallocateImpl, &deallocateImpl)
    {
    }

    static void* allocateImpl(memAllocator* inst, size_t size) /* SOUP_EXCAL */
    {
        //printf("allocate %zu\n", size);
        void* ptr = luaM_realloc_(static_cast<PlutoSingleBlockAllocator*>(inst)->L, nullptr, 0, size);
        SOUP_IF_LIKELY (ptr)
        {
            static_cast<PlutoSingleBlockAllocator*>(inst)->size = size;
            return ptr;
        }
        throw std::bad_alloc{};
    }

    static void* reallocateImpl(memAllocator* inst, void* addr, size_t new_size) /* SOUP_EXCAL */
    {
        //printf("resize %zu to %zu\n", static_cast<PlutoSingleBlockAllocator*>(inst)->size, new_size);
        addr = luaM_realloc_(static_cast<PlutoSingleBlockAllocator*>(inst)->L, addr, static_cast<PlutoSingleBlockAllocator*>(inst)->size, new_size);
        SOUP_IF_LIKELY (addr)
        {
            static_cast<PlutoSingleBlockAllocator*>(inst)->size = new_size;
            return addr;
        }
        throw std::bad_alloc{};
    }

    static void deallocateImpl(memAllocator* inst, void* addr) noexcept
    {
        luaM_free_(static_cast<PlutoSingleBlockAllocator*>(inst)->L, addr, static_cast<PlutoSingleBlockAllocator*>(inst)->size);
    }
};
#endif

struct PlutoBuffer
{
#ifdef PLUTO_MEMORY_LIMIT
    PlutoSingleBlockAllocator allocator;
#endif
    soup::Buffer<> buffer;

    PlutoBuffer()
#ifdef PLUTO_MEMORY_LIMIT
        : buffer(allocator)
#endif
    {
    }
};

[[nodiscard]] static PlutoBuffer* checkbuffer (lua_State *L, int i) {
  const auto buf = (PlutoBuffer*)luaL_checkudata(L, i, "pluto:buffer");
#ifdef PLUTO_MEMORY_LIMIT
  buf->allocator.L = L;
#endif
  return buf;
}

static int buffer_new (lua_State *L) {
  new (lua_newuserdata(L, sizeof(PlutoBuffer))) PlutoBuffer{};
  if (luaL_newmetatable(L, "pluto:buffer")) {
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:buffer\"", 28, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkbuffer(L, 1));
      return 0;
    });
    lua_settable(L, -3);
	lua_pushliteral(L, "__tostring");
	luaL_loadbuffer(L, "return require\"pluto:buffer\".tostring", 37, 0);
	lua_call(L, 0, 1);
	lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
  return 1;
}

static int buffer_append (lua_State *L) {
  size_t size;
  bool fail = false;
  const char* data = luaL_checklstring(L, 2, &size);
  try {
    checkbuffer(L, 1)->buffer.append(data, size);
  }
  catch (const std::bad_alloc&) {
    fail = true;
  }
  if (l_unlikely(fail))
    luaD_throw(L, LUA_ERRMEM);
  return 0;
}

static int buffer_tostring (lua_State *L) {
  soup::Buffer<>& buf = checkbuffer(L, 1)->buffer;
  bool fail = false;
  try {
    lua_pushlstring(L, (const char*)buf.data(), buf.size());
  }
  catch (const std::bad_alloc&) {
    fail = true;
  }
  if (l_unlikely(fail))
    luaD_throw(L, LUA_ERRMEM);
  return 1;
}

static const luaL_Reg funcs_buffer[] = {
  {"new", buffer_new},
  {"append", buffer_append},
  {"tostring", buffer_tostring},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(buffer)
