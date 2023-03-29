#pragma once
#ifdef PLUTO_USE_SOUP
// https://github.com/calamity-inc/Soup-Lua-Bindings/blob/main/soup_lua_bindings.hpp

#include "vendor/Soup/json.hpp"
#include "vendor/Soup/JsonInt.hpp"
#include "vendor/Soup/JsonBool.hpp"
#include "vendor/Soup/JsonNode.hpp"
#include "vendor/Soup/JsonFloat.hpp"
#include "vendor/Soup/JsonArray.hpp"
#include "vendor/Soup/JsonObject.hpp"
#include "vendor/Soup/JsonString.hpp"
#include "vendor/Soup/UniquePtr.hpp"

static bool isIndexBasedTable(lua_State* L, int i)
{
	lua_pushvalue(L, i);
	lua_pushnil(L);
	size_t k = 1;
	for (; lua_next(L, -2); ++k)
	{
		lua_pushvalue(L, -2);
		if (lua_type(L, -1) != LUA_TNUMBER)
		{
			lua_pop(L, 4);
			return false;
		}
		if (!lua_isinteger(L, -1))
		{
			lua_pop(L, 4);
			return false;
		}
		if (lua_tointeger(L, -1) != k)
		{
			lua_pop(L, 4);
			return false;
		}
		lua_pop(L, 2);
	}
	lua_pop(L, 1);
	return k != 1; // say it's not an index based table if it's empty
}

static soup::UniquePtr<soup::JsonNode> checkJson(lua_State* L, int i)
{
	auto type = lua_type(L, i);
	if (type == LUA_TBOOLEAN)
	{
		return soup::make_unique<soup::JsonBool>(lua_toboolean(L, i));
	}
	else if (type == LUA_TNUMBER)
	{
		if (lua_isinteger(L, i))
		{
			return soup::make_unique<soup::JsonInt>(lua_tointeger(L, i));
		}
		else
		{
			return soup::make_unique<soup::JsonFloat>(lua_tonumber(L, i));
		}
	}
	else if (type == LUA_TSTRING)
	{
		return soup::make_unique<soup::JsonString>(lua_tostring(L, i));
	}
	else if (type == LUA_TTABLE)
	{
		if (isIndexBasedTable(L, i))
		{
			auto arr = soup::make_unique<soup::JsonArray>();
			lua_pushvalue(L, i);
			lua_pushnil(L);
			while (lua_next(L, -2))
			{
				lua_pushvalue(L, -2);
				arr->children.emplace_back(checkJson(L, -2));
				lua_pop(L, 2);
			}
			lua_pop(L, 1);
			return arr;
		}
		else
		{
			auto obj = soup::make_unique<soup::JsonObject>();
			lua_pushvalue(L, i);
			lua_pushnil(L);
			while (lua_next(L, -2))
			{
				lua_pushvalue(L, -2);
				obj->children.emplace_back(checkJson(L, -1), checkJson(L, -2));
				lua_pop(L, 2);
			}
			lua_pop(L, 1);
			return obj;
		}
	}
	luaL_typeerror(L, i, "JSON-castable type");
}

static void pushFromJson(lua_State* L, const soup::JsonNode& node)
{
	if (node.isBool())
	{
		lua_pushboolean(L, node.asBool().value);
	}
	else if (node.isInt())
	{
		lua_pushinteger(L, node.asInt().value);
	}
	else if (node.isFloat())
	{
		lua_pushnumber(L, node.asFloat().value);
	}
	else if (node.isStr())
	{
		pluto_pushstring(L, node.asStr().value);
	}
	else if (node.isArr())
	{
		lua_newtable(L);
		lua_Integer i = 1;
		for (const auto& child : node.asArr().children)
		{
			lua_pushinteger(L, i++);
			pushFromJson(L, *child);
			lua_settable(L, -3);
		}
	}
	else if (node.isObj())
	{
		lua_newtable(L);
		for (const auto& e : node.asObj().children)
		{
			pushFromJson(L, *e.first);
			pushFromJson(L, *e.second);
			lua_settable(L, -3);
		}
	}
	else
	{
		lua_pushnil(L);
	}
}

#endif