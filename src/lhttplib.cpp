#define LUA_LIB
#include "lualib.h"

#include "lstate.h"

#include "vendor/Soup/soup/DetachedScheduler.hpp"
#include "vendor/Soup/soup/HttpRequest.hpp"
#include "vendor/Soup/soup/HttpRequestTask.hpp"
#include "vendor/Soup/soup/Uri.hpp"

static int push_http_response (lua_State *L, const std::optional<soup::HttpResponse>& resp) {
  if (resp.has_value()) {
    // Return value order should be 'body, status_code, response_headers, status_text' for compatibility with luasocket.
    pluto_pushstring(L, resp->body);
    lua_pushinteger(L, resp->status_code);
    return 2;
  }
  // In case of failure, luasocket would return 'nil, message' but I'm not sure what kind of message to give other than "connection failed."
  return 0;
}

static int http_request_cont (lua_State *L, int status, lua_KContext ctx) {
  auto pTask = reinterpret_cast<soup::HttpRequestTask*>(ctx);
  if (!pTask->isWorkDone()) {
    return lua_yieldk(L, 0, ctx, http_request_cont);
  }
  return push_http_response(L, *pTask->result);
}

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
  soup::HttpRequest hr(soup::Uri(std::move(uri)));
  if (optionsidx) {
    lua_pushliteral(L, "method");
    if (lua_rawget(L, optionsidx) > LUA_TNIL)
      hr.method = pluto_checkstring(L, -1);
    lua_pop(L, 1);
  }
  if (lua_isyieldable(L)) {
    if (G(L)->scheduler == nullptr) {
      G(L)->scheduler = new soup::DetachedScheduler();
    }

    auto upTask = reinterpret_cast<soup::DetachedScheduler*>(G(L)->scheduler)->add<soup::HttpRequestTask>(std::move(hr));
    auto pTask = upTask.get();

    auto pUpTask = new (lua_newuserdata(L, sizeof(upTask))) decltype(upTask) (std::move(upTask));
    lua_newtable(L);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(reinterpret_cast<decltype(upTask)*>(lua_touserdata(L, 1)));
      return 0;
    });
    lua_settable(L, -3);
    lua_setmetatable(L, -2);

    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(pTask), http_request_cont);
  }
  return push_http_response(L, hr.execute());
}

static const luaL_Reg funcs[] = {
  {"request", http_request},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(http);
