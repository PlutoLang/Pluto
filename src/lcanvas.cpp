#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/Canvas.hpp"
#include "vendor/Soup/soup/MemoryRefReader.hpp"
#include "vendor/Soup/soup/QrCode.hpp"

static soup::Canvas* checkcanvas (lua_State *L, int i) {
  return (soup::Canvas*)luaL_checkudata(L, i, "pluto:canvas");
}

static void pushcanvas (lua_State *L, soup::Canvas&& canvas) {
  new (lua_newuserdata(L, sizeof(soup::Canvas))) soup::Canvas(std::move(canvas));
  if (luaL_newmetatable(L, "pluto:canvas")) {
    lua_pushliteral(L, "__index");
    luaL_loadbuffer(L, "return require\"pluto:canvas\"", 28, 0);
    lua_call(L, 0, 1);
    lua_settable(L, -3);
    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, [](lua_State *L) {
      std::destroy_at<>(checkcanvas(L, 1));
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
}

static int canvas_new (lua_State* L) {
  const auto width = luaL_checkinteger(L, 1);
  const auto height = luaL_checkinteger(L, 2);
  pushcanvas(L, soup::Canvas(static_cast<unsigned int>(width), static_cast<unsigned int>(height)));
  return 1;
}

static int canvas_bmp (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  soup::MemoryRefReader r(data, size);
  pushcanvas(L, soup::Canvas::fromBmp(r));
  return 1;
}

static int canvas_qrcode (lua_State *L) {
  size_t size;
  const char *data = luaL_checklstring(L, 1, &size);
  soup::QrCode::ecc ecl = soup::QrCode::ecc::LOW;
  unsigned int border = 0;
  soup::Rgb fg = soup::Rgb::BLACK;
  soup::Rgb bg = soup::Rgb::WHITE;
  if (lua_gettop(L) >= 2) {
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_pushliteral(L, "ecl");
    if (lua_gettable(L, 2) > LUA_TNIL) {
      const char* const options[] = { "low", "medium", "quartile", "high", nullptr };
      ecl = static_cast<soup::QrCode::ecc>(luaL_checkoption(L, -1, "low", options));
    }

    lua_pushliteral(L, "border");
    if (lua_gettable(L, 2) > LUA_TNIL)
      border = static_cast<unsigned int>(luaL_checkinteger(L, -1));

    lua_pushliteral(L, "fg");
    if (lua_gettable(L, 2) > LUA_TNIL)
      fg = soup::Rgb(static_cast<uint32_t>(luaL_checkinteger(L, -1)));

    lua_pushliteral(L, "bg");
    if (lua_gettable(L, 2) > LUA_TNIL)
      bg = soup::Rgb(static_cast<uint32_t>(luaL_checkinteger(L, -1)));
  }

  try {
    std::string content(data, size);
    pushcanvas(L, soup::QrCode::encodeText(content, ecl).toCanvas(border, fg, bg));
  }
  catch (...) {
    luaL_error(L, "failed to encode qrcode");
  }
  return 1;
}

static int canvas_get (lua_State* L) {
  const auto c = checkcanvas(L, 1);
  const auto x = static_cast<unsigned int>(luaL_checkinteger(L, 2));
  const auto y = static_cast<unsigned int>(luaL_checkinteger(L, 3));
  if (l_unlikely(x >= c->width || y >= c->height)) {
    luaL_error(L, "out of bounds");
  }
  lua_pushinteger(L, c->get(x, y).toInt());
  return 1;
}

static int canvas_set (lua_State* L) {
  const auto c = checkcanvas(L, 1);
  const auto x = static_cast<unsigned int>(luaL_checkinteger(L, 2));
  const auto y = static_cast<unsigned int>(luaL_checkinteger(L, 3));
  const auto v = soup::Rgb(static_cast<uint32_t>(luaL_checkinteger(L, 4)));
  if (l_unlikely(x >= c->width || y >= c->height)) {
    luaL_error(L, "out of bounds");
  }
  c->set(x, y, v);
  return 0;
}

static int canvas_fill (lua_State* L) {
  const auto c = checkcanvas(L, 1);
  const auto v = soup::Rgb(static_cast<uint32_t>(luaL_checkinteger(L, 2)));
  c->fill(v);
  return 0;
}

static int canvas_size (lua_State *L) {
  const auto c = checkcanvas(L, 1);
  lua_pushinteger(L, c->width);
  lua_pushinteger(L, c->height);
  return 2;
}

static int canvas_mulsize (lua_State *L) {
  const auto c = checkcanvas(L, 1);
  const auto x = static_cast<unsigned int>(luaL_checkinteger(L, 2));
  if (l_unlikely(x < 2))
    luaL_error(L, "multiplier must be at least 2");
  c->resizeNearestNeighbour(c->width * x, c->height * x);
  return 0;
}

static int canvas_tobmp (lua_State* L) {
  pluto_pushstring(L, checkcanvas(L, 1)->toBmp());
  return 1;
}

static int canvas_topng (lua_State* L) {
  pluto_pushstring(L, checkcanvas(L, 1)->toPng());
  return 1;
}

static int canvas_tobwstring(lua_State* L) {
  pluto_pushstring(L, checkcanvas(L, 1)->toStringDownsampledDoublewidthUtf8(true, false, soup::Rgb(static_cast<uint32_t>(luaL_checkinteger(L, 2)))));
  return 1;
}

static const luaL_Reg funcs_canvas[] = {
  {"new", canvas_new},
  {"bmp", canvas_bmp},
  {"qrcode", canvas_qrcode},
  {"get", canvas_get},
  {"set", canvas_set},
  {"fill", canvas_fill},
  {"size", canvas_size},
  {"mulsize", canvas_mulsize},
  {"tobmp", canvas_tobmp},
  {"topng", canvas_topng},
  {"tobwstring", canvas_tobwstring},
  {nullptr, nullptr}
};
PLUTO_NEWLIB(canvas);
