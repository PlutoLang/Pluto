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
    // Return value order is 'body, status_code, response_headers, status_text' for compatibility with luasocket.
    pluto_pushstring(L, task.result->body);
    lua_pushinteger(L, task.result->status_code);
    lua_newtable(L);
    for (auto& e : task.result->header_fields) {
      pluto_pushstring(L, e.first);
      pluto_pushstring(L, e.second);
      lua_settable(L, -3);
    }
    pluto_pushstring(L, task.result->status_text);
    return 4;
  }
  // Return value order is 'nil, message' for compatibility with luasocket.
  luaL_pushfail(L);
#if SOUP_WASM
  return 1;  /* specialized HttpRequestTask for WASM doesn't have `getStatus` */
#else
  lua_pushstring(L, soup::netStatusToString(task.getStatus()));
  return 2;
#endif
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

#if SOUP_WASM
  if (!lua_isyieldable(L)) {
    luaL_error(L, "HTTP requests are not possible in this WASM environment due to running Pluto as a blocking program.");
  }
#endif

  soup::HttpRequest hr(soup::Uri(std::move(uri)));
  if (optionsidx) {
    lua_pushnil(L);
    while (lua_next(L, optionsidx)) {
      if (lua_type(L, -2) != LUA_TSTRING) {
        pluto_warning(L, luaO_pushfstring(L, "unrecognized http request option: %s", lua_tostring(L, -2)));
        lua_pop(L, 1);
      }
      const char *str = lua_tostring(L, -2);
      if (strcmp(str, "url") != 0 && strcmp(str, "method") != 0 && strcmp(str, "headers") != 0 && strcmp(str, "body") != 0 && strcmp(str, "prefer_ipv6") != 0) {
        pluto_warning(L, luaO_pushfstring(L, "unrecognized http request option: %s", lua_tostring(L, -2)));
        lua_pop(L, 1);
      }
      lua_pop(L, 1);
    }

    lua_pushliteral(L, "method");
    if (lua_rawget(L, optionsidx) > LUA_TNIL)
      hr.method = pluto_checkstring(L, -1);
    lua_pop(L, 1);
    lua_pushliteral(L, "headers");
    if (lua_rawget(L, optionsidx) > LUA_TNIL) {
      lua_pushnil(L);
      while (lua_next(L, -2)) {
        hr.setHeader(pluto_checkstring(L, -2), pluto_checkstring(L, -1));
        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
    lua_pushliteral(L, "body");
    if (lua_rawget(L, optionsidx) > LUA_TNIL)
      hr.setPayload(pluto_checkstring(L, -1));
    else if (hr.method != "GET")
      hr.setPayload("");
    lua_pop(L, 1);
  }

#if SOUP_WASM
  auto pTask = new (lua_newuserdata(L, sizeof(soup::HttpRequestTask))) soup::HttpRequestTask(std::move(hr));
  lua_newtable(L);
  lua_pushliteral(L, "__gc");
  lua_pushcfunction(L, [](lua_State *L) {
    std::destroy_at<>(reinterpret_cast<soup::HttpRequestTask*>(lua_touserdata(L, 1)));
    return 0;
  });
  lua_settable(L, -3);
  lua_setmetatable(L, -2);
  return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(pTask), http_request_cont);
#else
  if (G(L)->scheduler == nullptr) {
    G(L)->scheduler = new soup::DetachedScheduler();
  }
  auto spTask = reinterpret_cast<soup::DetachedScheduler*>(G(L)->scheduler)->add<soup::HttpRequestTask>(std::move(hr));
  if (optionsidx) {
    lua_pushliteral(L, "prefer_ipv6");
    if (lua_rawget(L, optionsidx) > LUA_TNIL)
      spTask->prefer_ipv6 = lua_istrue(L, -1);
    lua_pop(L, 1);
  }
  if (lua_isyieldable(L)) {
    auto pTask = spTask.get();

    new (lua_newuserdata(L, sizeof(spTask))) decltype(spTask) (std::move(spTask));
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
#endif
}

static const luaL_Reg funcs[] = {
  {"request", http_request},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(http);
