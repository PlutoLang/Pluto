#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/HttpRequest.hpp"
#include "vendor/Soup/soup/HttpRequestTask.hpp"
#include "vendor/Soup/soup/Scheduler.hpp"
#include "vendor/Soup/soup/Uri.hpp"

struct HttpRequestState {
  soup::Scheduler sched; // TOOD: Move this into registry (with appropriate __gc metamethod) so we can reuse connections.
  soup::HttpRequestTask* task;
};

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
  auto state = reinterpret_cast<HttpRequestState*>(ctx);
  if (!state->task->isWorkDone()) {
    state->sched.tick(); // TODO: The Scheduler should NOT be our responsibility.
    return lua_yieldk(L, 0, ctx, http_request_cont);
  }
  int nresults = push_http_response(L, *state->task->result);
  delete state; // Not very Lua-like. We should GC. :megamind:
  return nresults;
}

static int http_request (lua_State *L) {
  soup::HttpRequest hr(soup::Uri(luaL_checkstring(L, 1)));
  if (lua_isyieldable(L)) {
    auto state = new HttpRequestState{};
    state->task = state->sched.add<soup::HttpRequestTask>(std::move(hr));
    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(state), http_request_cont);
  }
  return push_http_response(L, hr.execute());
}

static const luaL_Reg funcs[] = {
  {"request", http_request},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(http);
