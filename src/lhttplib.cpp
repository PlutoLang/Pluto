#define LUA_LIB
#include "lualib.h"

#include "lstate.h"

#include <thread>

#include "vendor/Soup/soup/DetachedScheduler.hpp"
#include "vendor/Soup/soup/HttpRequest.hpp"
#include "vendor/Soup/soup/HttpRequestTask.hpp"
#include "vendor/Soup/soup/netStatus.hpp"
#include "vendor/Soup/soup/Uri.hpp"

static int push_http_response (lua_State *L, soup::HttpRequestTask& task) {
  if (task.result.has_value()) {
    // Return value order should be 'body, status_code, response_headers, status_text' for compatibility with luasocket.
    pluto_pushstring(L, task.result->body);
    lua_pushinteger(L, task.result->status_code);
    return 2;
  }
  // Return value order should be 'nil, message' for compatibility with luasocket.
  luaL_pushfail(L);
  lua_pushstring(L, soup::netStatusToString(task.getStatus()));
  return 2;
}

static int http_request_cont (lua_State *L, int status, lua_KContext ctx) {
  auto pTask = reinterpret_cast<soup::HttpRequestTask*>(ctx);
  if (!pTask->isWorkDone()) {
    return lua_yieldk(L, 0, ctx, http_request_cont);
  }
  return push_http_response(L, *pTask);
}

#ifdef PLUTO_HTTP_REQUEST_HOOK
extern bool PLUTO_HTTP_REQUEST_HOOK(lua_State* L, const char* url);
#endif

static int http_request (lua_State *L) {
  std::string uri;
  int optionsidx = 0;
  if (lua_type(L, 1) == LUA_TTABLE) {
    lua_pushliteral(L, "url");
    if (lua_rawget(L, 1) != LUA_TSTRING)
      luaL_error(L, "Table is missing 'url' option");
    uri = pluto_checkstring(L, -1);
    lua_pop(L, 1);
    optionsidx = 1;
  }
  else {
    uri = pluto_checkstring(L, 1);
    if (lua_type(L, 2) == LUA_TTABLE)
      optionsidx = 2;
  }
#ifdef PLUTO_DISABLE_HTTP_COMPLETELY
  luaL_error(L, "disallowed by content moderation policy");
#endif
#ifdef PLUTO_HTTP_REQUEST_HOOK
  if (!PLUTO_HTTP_REQUEST_HOOK(L, uri.c_str()))
    luaL_error(L, "disallowed by content moderation policy");
#endif
  soup::HttpRequest hr(soup::Uri(std::move(uri)));
  if (optionsidx) {
    lua_pushliteral(L, "method");
    if (lua_rawget(L, optionsidx) > LUA_TNIL)
      hr.method = pluto_checkstring(L, -1);
    lua_pop(L, 1);
  }
  if (G(L)->scheduler == nullptr) {
    G(L)->scheduler = new soup::DetachedScheduler();
  }
  auto spTask = reinterpret_cast<soup::DetachedScheduler*>(G(L)->scheduler)->add<soup::HttpRequestTask>(std::move(hr));
  if (lua_isyieldable(L)) {
    auto pTask = spTask.get();

    auto pUpTask = new (lua_newuserdata(L, sizeof(spTask))) decltype(spTask) (std::move(spTask));
    lua_newtable(L);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(reinterpret_cast<decltype(spTask)*>(lua_touserdata(L, 1)));
      return 0;
    });
    lua_settable(L, -3);
    lua_setmetatable(L, -2);

    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(pTask), http_request_cont);
  }
  while (!spTask->isWorkDone())
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  return push_http_response(L, *spTask);
}

static const luaL_Reg funcs[] = {
  {"request", http_request},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(http);
