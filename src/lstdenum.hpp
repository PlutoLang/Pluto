#pragma once

#include <vector>

namespace StdEnum {
	static void registerEnum(lua_State* L, const char* name, std::vector<const char*>&& values, lua_Integer enum_start_v = 1) {
		lua_newtable(L);

		for (const char* value_name : values) {
			lua_pushinteger(L, enum_start_v++);
			lua_setfield(L, -2, value_name);
		}

		lua_setfield(L, -2, name);
	}
}