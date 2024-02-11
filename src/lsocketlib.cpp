#ifndef __EMSCRIPTEN__

#define LUA_LIB
#include "lualib.h"
#include "lstate.h"

#include <deque>
#include <thread>

#include "vendor/Soup/soup/netConnectTask.hpp"
#include "vendor/Soup/soup/Scheduler.hpp"
#include "vendor/Soup/soup/Socket.hpp"

struct StandaloneSocket {
  soup::Scheduler sched;
  soup::SharedPtr<soup::Socket> sock;
  std::deque<std::string> recvd;
  bool did_tls_handshake = false;

  void recvLoop() SOUP_EXCAL {
    sock->recv([](soup::Socket&, std::string&& data, soup::Capture&& cap) SOUP_EXCAL {
      StandaloneSocket& ss = *cap.get<StandaloneSocket*>();
      ss.recvd.push_back(std::move(data));
      if (!ss.sock->remote_closed)
        ss.recvLoop();
    }, this);
  }
};

static StandaloneSocket* checksocket (lua_State *L, int i) {
  return (StandaloneSocket*)luaL_checkudata(L, i, "pluto:socket");
}

static int connectcont (lua_State *L, int status, lua_KContext ctx) {
  StandaloneSocket& ss = *checksocket(L, -1);
  auto pTask = reinterpret_cast<soup::netConnectTask*>(ctx);
  if (pTask->isWorkDone()) {
    if (!pTask->wasSuccessful()) {
      return 0;
    }
    ss.sock = pTask->getSocket(ss.sched);
    ss.recvLoop();
    ss.sched.tick();  /* get rid of netConnectTask */
    return 1;
  }
  ss.sched.tick();
  return lua_yieldk(L, 0, ctx, connectcont);
}

static int l_connect (lua_State *L) {
  const char *host = luaL_checkstring(L, 1);
  auto port = static_cast<uint16_t>(luaL_checkinteger(L, 2));

  if (luaL_newmetatable(L, "pluto:socket")) {
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:socket\"", 28, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checksocket(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_pop(L, 1);

  StandaloneSocket& ss = *new (lua_newuserdata(L, sizeof(StandaloneSocket))) StandaloneSocket{};
  luaL_setmetatable(L, "pluto:socket");

  if (!lua_isyieldable(L)) {
    ss.sock = ss.sched.addSocket();
    bool connected = ss.sock->connect(host, port);
    if (!connected)
      return 0;
    ss.recvLoop();
    return 1;
  }

  auto spTask = ss.sched.add<soup::netConnectTask>(host, port);
  ss.sched.tick();
  return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(spTask.get()), connectcont);
}

static int l_send (lua_State *L) {
  checksocket(L, 1)->sock->send(pluto_checkstring(L, 2));
  return 0;
}

static int restrecv (lua_State *L, StandaloneSocket& ss) {
  if (!ss.recvd.empty()) {
    pluto_pushstring(L, std::move(ss.recvd.front()));
    ss.recvd.pop_front();
    return 1;
  }
  return 0;
}

static int recvcont (lua_State *L, int status, lua_KContext ctx) {
  StandaloneSocket& ss = *reinterpret_cast<StandaloneSocket*>(ctx);
  if (ss.recvd.empty()) {
    ss.sched.tick();
    if (!ss.sock->isWorkDone()) {
      return lua_yieldk(L, 0, ctx, recvcont);
    }
  }
  return restrecv(L, ss);
}

static int l_recv (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  
  if (lua_isyieldable(L))
    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(&ss), recvcont);

  if (ss.recvd.empty()) {
    ss.sched.tick();
    while (ss.recvd.empty() && !ss.sock->isWorkDone()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      ss.sched.tick();
    }
  }
  return restrecv(L, ss);
}

static int unrecv (lua_State *L) {
  checksocket(L, 1)->recvd.push_front(pluto_checkstring(L, 2));
  return 0;
}

static int starttlscont (lua_State *L, int status, lua_KContext ctx) {
  StandaloneSocket& ss = *reinterpret_cast<StandaloneSocket*>(ctx);
  ss.sched.tick();
  if (l_likely(!ss.did_tls_handshake && !ss.sock->isWorkDone()))
    return lua_yieldk(L, 0, ctx, starttlscont);
  lua_pushboolean(L, ss.did_tls_handshake);
  return 1;
}

static int starttls (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  if (ss.did_tls_handshake)
    return 0;
  ss.sock->enableCryptoClient(luaL_checkstring(L, 2), [](soup::Socket&, soup::Capture&& cap) SOUP_EXCAL {
    StandaloneSocket& ss = *cap.get<StandaloneSocket*>();
    ss.did_tls_handshake = true;
    ss.recvLoop();  /* re-add recv loop, now on crypto layer */
  }, &ss);

  if (lua_isyieldable(L))
    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(&ss), starttlscont);

  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ss.sched.tick();
  } while (!ss.did_tls_handshake && !ss.sock->isWorkDone());
  lua_pushboolean(L, ss.did_tls_handshake);
  return 1;
}

static const luaL_Reg funcs[] = {
  {"connect", l_connect},
  {"send", l_send},
  {"recv", l_recv},
  {"unrecv", unrecv},
  {"starttls", starttls},
  {NULL, NULL}
};

PLUTO_NEWLIB(socket)

#endif
