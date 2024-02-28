#pragma once

#include <algorithm>

#include "vendor/Soup/soup/json.hpp"
#include "vendor/Soup/soup/JsonInt.hpp"
#include "vendor/Soup/soup/JsonBool.hpp"
#include "vendor/Soup/soup/JsonNode.hpp"
#include "vendor/Soup/soup/JsonFloat.hpp"
#include "vendor/Soup/soup/JsonArray.hpp"
#include "vendor/Soup/soup/JsonObject.hpp"
#include "vendor/Soup/soup/JsonString.hpp"
#include "vendor/Soup/soup/UniquePtr.hpp"

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
		return soup::make_unique<soup::JsonString>(pluto_checkstring(L, i));
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
			if (auto itOrder = obj->findIt("__order"); itOrder != obj->end())
			{
				if (itOrder->second->isArr())
				{
					auto upOrder = std::move(itOrder->second);
					obj->erase(itOrder);
					std::sort(obj->begin(), obj->end(), [pOrder{ static_cast<soup::JsonArray*>(upOrder.get()) }]
						(const soup::JsonObject::Container::value_type& a, const soup::JsonObject::Container::value_type& b)
					{
						for (const auto& val : *pOrder)
						{
							if (*a.first == val)
							{
								return true;
							}
							if (*b.first == val)
							{
								return false;
							}
						}
						return false;
					});
				}
			}
			return obj;
		}
	}
	else if (type == LUA_TLIGHTUSERDATA)
	{
		if (reinterpret_cast<uintptr_t>(lua_touserdata(L, i)) == 'PJNL')
		{
			return soup::make_unique<soup::JsonNull>();
		}
	}
	luaL_typeerror(L, i, "JSON-castable type");
}

static void pushFromJson(lua_State* L, const soup::JsonNode& node, int flags)
{
	switch (node.type)
	{
	case soup::JSON_BOOL:
		lua_pushboolean(L, node.reinterpretAsBool().value);
		break;

	case soup::JSON_INT:
		lua_pushinteger(L, node.reinterpretAsInt().value);
		break;

	case soup::JSON_FLOAT:
		lua_pushnumber(L, node.reinterpretAsFloat().value);
		break;

	case soup::JSON_STRING:
		pluto_pushstring(L, node.reinterpretAsStr().value);
		break;

	case soup::JSON_ARRAY:
		{
			lua_newtable(L);
			lua_Integer i = 1;
			for (const auto& child : node.reinterpretAsArr().children)
			{
				lua_pushinteger(L, i++);
				pushFromJson(L, *child, flags);
				lua_settable(L, -3);
			}
		}
		break;

	case soup::JSON_OBJECT:
		{
			lua_newtable(L);
			for (const auto& e : node.reinterpretAsObj().children)
			{
				pushFromJson(L, *e.first, flags);
				pushFromJson(L, *e.second, flags);
				lua_settable(L, -3);
			}
			if (flags & (1 << 1))
			{
				lua_pushliteral(L, "__order");
				lua_newtable(L);
				lua_Integer i = 1;
				for (const auto& e : node.reinterpretAsObj().children)
				{
					lua_pushinteger(L, i++);
					pushFromJson(L, *e.first, flags);
					lua_settable(L, -3);
				}
				lua_settable(L, -3);
			}
		}
		break;

	case soup::JSON_NULL:
		if (flags & (1 << 0))
		{
			lua_pushlightuserdata(L, reinterpret_cast<void*>(static_cast<uintptr_t>(0xF01D)));
		}
		else
		{
			lua_pushnil(L);
		}
		break;
	}
}
