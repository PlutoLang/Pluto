#include <cstring> // strcmp

#define LUA_LIB
#include "lualib.h"
#include "lstate.h" // luaE_incCstack

#include "vendor/Soup/soup/xml.hpp"

static void check_xml (lua_State *L, int i, soup::UniquePtr<soup::XmlNode>& out) {
  const auto type = lua_type(L, i);
  if (type == LUA_TTABLE) {
    lua_checkstack(L, 4);
    lua_pushvalue(L, i);
    lua_pushliteral(L, "tag");
    if (lua_rawget(L, -2) == LUA_TSTRING) {
      luaE_incCstack(L);
      out = soup::make_unique<soup::XmlTag>();
      auto& tag = static_cast<soup::XmlTag&>(*out);
      tag.name = pluto_checkstring(L, -1);
      lua_pop(L, 1);  /* pop result of lua_rawget */
      lua_pushliteral(L, "attributes");
      if (lua_rawget(L, -2) != LUA_TNONE) {
        if (lua_type(L, -1) == LUA_TTABLE) {
          lua_pushnil(L);
          while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            tag.attributes.emplace_back(pluto_checkstring(L, -1), pluto_checkstring(L, -2));
            lua_pop(L, 2);
          }
        }
        lua_pop(L, 1);  /* pop result of lua_rawget */
      }
      lua_pushliteral(L, "children");
      if (lua_rawget(L, -2) != LUA_TNONE) {
        if (lua_type(L, -1) == LUA_TTABLE) {
          lua_pushnil(L);
          while (lua_next(L, -2)) {
            check_xml(L, -1, tag.children.emplace_back());
            lua_pop(L, 1);
          }
        }
        lua_pop(L, 1);  /* pop result of lua_rawget */
      }
      lua_pop(L, 1);  /* pop table from lua_pushvalue */
      L->nCcalls--;
      return;
    }
  }
  else if (type == LUA_TSTRING) {
    out = soup::make_unique<soup::XmlText>(pluto_checkstring(L, i));
    return;
  }
  luaL_typeerror(L, i, "XML-castable type");
}

static int xml_encode (lua_State *L) {
  auto& root = *pluto_newclassinst(L, soup::UniquePtr<soup::XmlNode>);
  check_xml(L, 1, root);
  if (lua_istrue(L, 2)) {
    pluto_pushstring(L, root->encodePretty());
  }
  else {
    pluto_pushstring(L, root->encode());
  }
  return 1;
}

static void pushxmltag (lua_State *L, const soup::XmlTag& tag) {
  lua_checkstack(L, 5);
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

  if (luaL_newmetatable(L, "pluto:xml_full_node")) {
    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, [](lua_State *L) -> int {
      lua_pushliteral(L, "children");
      if (lua_rawget(L, 1) > LUA_TNIL) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
          if (lua_type(L, -1) == LUA_TTABLE) {
            lua_pushliteral(L, "tag");
            lua_rawget(L, -2);
            if (lua_compare(L, 2, -1, LUA_OPEQ)) {
              lua_pop(L, 1);
              return 1;
            }
            lua_pop(L, 1);
          }
          lua_pop(L, 1);
        }
      }
      return 0;
    });
    lua_settable(L, -3);
  }
  lua_setmetatable(L, -2);
}

static int xml_decode (lua_State *L) {
  const soup::XmlMode *mode = &soup::xml::MODE_XML;
  if (lua_gettop(L) >= 2) {
    const char *modename = luaL_checkstring(L, 2);
    if (strcmp(modename, "html") == 0)
      mode = &soup::xml::MODE_HTML;
    else if (strcmp(modename, "lax") == 0)
      mode = &soup::xml::MODE_LAX_XML;
    else if (strcmp(modename, "xml") != 0)
      luaL_error(L, "unknown parser mode '%s'", modename);
  }
  size_t len;
  const char *data = luaL_checklstring(L, 1, &len);
  std::string* what = nullptr;
  soup::UniquePtr<soup::XmlTag> root;
  try {
    root = soup::xml::parseAndDiscardMetadata(data, data + len, *mode);
  }
  catch (const std::exception& e) {
    what = pluto_newclassinst(L, std::string);
    *what = e.what();
  }
  if (!root) {
    luaL_error(L, what->c_str());
  }
  lua_assert(!what);
  pushxmltag(L, *root);
  return 1;
}

static const luaL_Reg funcs_xml[] = {
  {"encode", xml_encode},
  {"decode", xml_decode},
  {nullptr, nullptr}
};

PLUTO_NEWLIB(xml)
