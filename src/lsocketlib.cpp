#ifndef __EMSCRIPTEN__

#define LUA_LIB
#include "lualib.h"
#include "lstate.h"

#include <deque>
#include <queue>
#include <thread>

#include "vendor/Soup/soup/CertStore.hpp"
#include "vendor/Soup/soup/netConnectTask.hpp"
#include "vendor/Soup/soup/Scheduler.hpp"
#include "vendor/Soup/soup/Server.hpp"
#include "vendor/Soup/soup/ServerService.hpp"
#include "vendor/Soup/soup/Socket.hpp"

struct StandaloneSocket {
  soup::Scheduler sched;
  soup::SharedPtr<soup::Socket> sock;
  std::deque<std::string> recvd;
  bool did_tls_handshake = false;
  bool from_listener = false;

  void recvLoop() SOUP_EXCAL {
    sock->recv([](soup::Socket&, std::string&& data, soup::Capture&& cap) SOUP_EXCAL {
      StandaloneSocket& ss = *cap.get<StandaloneSocket*>();
      ss.recvd.push_back(std::move(data));
      ss.recvLoop();
    }, this);
  }
};

static StandaloneSocket* checksocket (lua_State *L, int i) {
  return (StandaloneSocket*)luaL_checkudata(L, i, "pluto:socket");
}

static StandaloneSocket& pushsocket (lua_State *L) {
  StandaloneSocket& ss = *new (lua_newuserdata(L, sizeof(StandaloneSocket))) StandaloneSocket{};
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
  lua_setmetatable(L, -2);
  return ss;
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

  StandaloneSocket& ss = pushsocket(L);

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
  size_t len;
  const char *str = luaL_checklstring(L, 2, &len);
  checksocket(L, 1)->sock->send(str, len);
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

static int l_peek (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  ss.sched.tick();
  if (!ss.recvd.empty()) {
    pluto_pushstring(L, ss.recvd.front());
    return 1;
  }
  return 0;
}

static int l_recv (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  ss.sched.tick();
  if (ss.recvd.empty()) {
    if (lua_isyieldable(L))
      return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(&ss), recvcont);
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

static void starttlscallback (soup::Socket&, soup::Capture&& cap) SOUP_EXCAL {
  StandaloneSocket& ss = *cap.get<StandaloneSocket*>();
  ss.did_tls_handshake = true;
  ss.recvLoop();  /* re-add recv loop, now on crypto layer */
}

static int starttls (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);

  if (ss.did_tls_handshake)
    return 0;

  if (ss.from_listener) {
    auto certstore = pluto_newclassinst(L, soup::SharedPtr<soup::CertStore>, new soup::CertStore());
    lua_pushnil(L);
    while (lua_next(L, 2)) {
      lua_pushliteral(L, "chain");
      lua_gettable(L, -2);
      size_t chain_len;
      const char *chain = luaL_checklstring(L, -1, &chain_len);
      lua_pop(L, 1);
      
      lua_pushliteral(L, "private_key");
      lua_gettable(L, -2);
      size_t privkey_len;
      const char *privkey = luaL_checklstring(L, -1, &privkey_len);
      lua_pop(L, 1);

      soup::X509Certchain chainstruct;
      chainstruct.fromPem(std::string(chain, chain_len));
      (*certstore)->add(std::move(chainstruct), soup::RsaPrivateKey::fromPem(std::string(privkey, privkey_len)));

      lua_pop(L, 1);
    }

    /* We may have already consumed the client_hello, so we need to give it back to Soup. */
    while (!ss.recvd.empty()) {
      ss.sock->transport_unrecv(ss.recvd.back());
      ss.recvd.pop_back();
    }

    ss.sock->enableCryptoServer(std::move(*certstore), starttlscallback, &ss);
  }
  else {
    ss.sock->enableCryptoClient(luaL_checkstring(L, 2), starttlscallback, &ss);
  }

  if (lua_isyieldable(L))
    return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(&ss), starttlscont);

  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ss.sched.tick();
  } while (!ss.did_tls_handshake && !ss.sock->isWorkDone());
  lua_pushboolean(L, ss.did_tls_handshake);
  return 1;
}

static int socket_istls (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  lua_pushboolean(L, ss.did_tls_handshake);
  return 1;
}

static int socket_close (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  ss.sock->close();
  return 0;
}

static int socket_isopen (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  lua_pushboolean(L, !ss.sock->isWorkDoneOrClosed());
  return 1;
}

static int socket_getside (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  if (ss.from_listener)
    lua_pushliteral(L, "server");
  else
    lua_pushliteral(L, "client");
  return 1;
}

static int socket_getpeer (lua_State *L) {
  StandaloneSocket& ss = *checksocket(L, 1);
  auto ipstr = ss.sock->peer.ip.toString();
  if (!ss.sock->peer.ip.isV4()) {
    ipstr.insert(0, 1, '[');
    ipstr.push_back(']');
  }
  pluto_pushstring(L, std::move(ipstr));
  lua_pushinteger(L, ss.sock->peer.getPort());
  return 2;
}

struct Listener {
  soup::Server serv;
  soup::ServerService srv{ &onTunnelEstablished };
  soup::SharedPtr<soup::Socket> accepted;

  static void onTunnelEstablished(soup::Socket&, soup::ServerService&, soup::Server& serv) SOUP_EXCAL {
    auto l = reinterpret_cast<Listener*>(&serv);
    auto sock = l->serv.pending_workers.pop_front();  /* a bit dirty, but it works. */
    l->accepted = std::move(*sock);
  }
};

static Listener* checklistener (lua_State *L, int i) {
  return (Listener*)luaL_checkudata(L, i, "pluto:socket-listener");
}

static int restaccept (lua_State *L, Listener& l) {
  auto& ss = pushsocket(L);
  ss.sock = l.accepted;
  ss.sched.addSocket(std::move(l.accepted));
  ss.from_listener = true;
  ss.recvLoop();
  l.serv.tick();  /* avoid having the to yield/block for the next call to accept */
  return 1;
}

static int acceptcont (lua_State *L, int status, lua_KContext ctx) {
  auto& l = *reinterpret_cast<Listener*>(ctx);
  if (!l.accepted) {
    l.serv.tick();
    return lua_yieldk(L, 0, ctx, acceptcont);
  }
  return restaccept(L, l);
}

static int listener_accept (lua_State *L) {
  auto& l = *checklistener(L, 1);
  if (!l.accepted) {
    if (lua_isyieldable(L)) {
      l.serv.tick();
      return lua_yieldk(L, 0, reinterpret_cast<lua_KContext>(&l), acceptcont);
    }
    do {
      l.serv.tick();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!l.accepted);
  }
  return restaccept(L, l);
}

static int listener_hasconnection (lua_State *L) {
  auto& l = *checklistener(L, 1);
  if (l.accepted) {
    lua_pushboolean(L, true);
  }
  else {
    lua_pushboolean(L, false);
    l.serv.tick();
  }
  return 1;
}

static int l_listen (lua_State *L) {
  auto port = static_cast<uint16_t>(luaL_checkinteger(L, 1));

  Listener& l = *new (lua_newuserdata(L, sizeof(Listener))) Listener{};
  if (luaL_newmetatable(L, "pluto:socket-listener")) {
    lua_pushliteral(L, "__index");
    lua_newtable(L);
    lua_pushliteral(L, "accept");
    lua_pushcfunction(L, listener_accept);
    lua_settable(L, -3);
    lua_pushliteral(L, "hasconnection");
    lua_pushcfunction(L, listener_hasconnection);
    lua_settable(L, -3);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checklistener(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);

  return l.serv.bind(port, &l.srv) ? 1 : 0;
}

static const luaL_Reg funcs_socket[] = {
  {"connect", l_connect},
  {"send", l_send},
  {"peek", l_peek},
  {"recv", l_recv},
  {"unrecv", unrecv},
  {"starttls", starttls},
  {"istls", socket_istls},
  {"close", socket_close},
  {"isopen", socket_isopen},
  {"getside", socket_getside},
  {"getpeer", socket_getpeer},
  {"listen", l_listen},
  {NULL, NULL}
};

LUAMOD_API int luaopen_socket (lua_State *L) {
  luaL_newlib(L, funcs_socket);

#ifndef PLUTO_DONT_LOAD_ANY_STANDARD_LIBRARY_CODE_WRITTEN_IN_PLUTO
  lua_pushliteral(L, "bind");
  luaL_loadstring(L, R"EOC(
return function(sched, port, callback)
    sched:add(function()
        local l = require"pluto:socket".listen(port)
        assert(l, "Failed to bind port "..port)
        while s := l:accept() do
            sched:add(function()
                callback(s)
            end)
        end
    end)
end)EOC");
  lua_call(L, 0, 1);
  lua_settable(L, -3);
#endif

  return 1;
}
const Pluto::PreloadedLibrary Pluto::preloaded_socket{ "socket", funcs_socket, &luaopen_socket };

#endif
