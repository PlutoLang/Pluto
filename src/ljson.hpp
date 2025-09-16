#pragma once

inline bool isIndexBasedTable(lua_State* L, int i)
{
	lua_pushvalue(L, i);
	lua_pushnil(L);
	size_t k = 1;
	for (; lua_next(L, -2); ++k)
	{
		if (lua_type(L, -2) != LUA_TNUMBER)
		{
			lua_pop(L, 3);
			return false;
		}
		if (!lua_isinteger(L, -2))
		{
			lua_pop(L, 3);
			return false;
		}
		if (lua_tointeger(L, -2) != k)
		{
			lua_pop(L, 3);
			return false;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return true;
}
