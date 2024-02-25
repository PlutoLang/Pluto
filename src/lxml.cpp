#define LUA_LIB
#include "lualib.h"

#include "vendor/Soup/soup/xml.hpp"

static void pushxmltag (lua_State *L, const soup::XmlTag& tag) {
  lua_newtable(L);
  lua_pushliteral(L, "tag");
  pluto_pushstring(L, tag.name);
  lua_settable(L, -3);
  if (!tag.attributes.empty()) {
    lua_pushliteral(L, "attributes");
    lua_newtable(L);
    for (const auto& attr : tag.attributes) {
      pluto_pushstring(L, attr.first);
      pluto_pushstring(L, attr.second);
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
  if (!tag.children.empty()) {
    lua_pushliteral(L, "children");
    lua_newtable(L);
    lua_Integer i = 1;
    for (const auto& child : tag.children) {
      lua_pushinteger(L, i++);
      if (child->isTag()) {
        pushxmltag(L, child->asTag());
      }
      else /*if (child->isText())*/ {
        pluto_pushstring(L, child->asText().contents);
      }
      lua_settable(L, -3);
    }
    lua_settable(L, -3);
  }
}

static int xml_decode (lua_State *L) {
  std::string data = pluto_checkstring(L, 1);
  auto root = soup::xml::parseAndDiscardMetadata(data);
  pushxmltag(L, *root);
  return 1;
}

static const luaL_Reg funcs[] = {
  {"decode", xml_decode},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(xml)
