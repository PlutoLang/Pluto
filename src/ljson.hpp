#pragma once

#include <algorithm>
#include <cmath> // isfinite

#include "vendor/Soup/soup/json.hpp"
#include "vendor/Soup/soup/JsonInt.hpp"
#include "vendor/Soup/soup/JsonBool.hpp"
#include "vendor/Soup/soup/JsonNode.hpp"
#include "vendor/Soup/soup/JsonFloat.hpp"
#include "vendor/Soup/soup/JsonArray.hpp"
#include "vendor/Soup/soup/JsonObject.hpp"
#include "vendor/Soup/soup/JsonString.hpp"
#include "vendor/Soup/soup/UniquePtr.hpp"

#include "lstate.h" // luaE_incCstack

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
	return true;
}
