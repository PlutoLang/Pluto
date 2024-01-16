#define LUA_LIB

#include "lauxlib.h"
#include "lualib.h"

#include <filesystem>

#include "ljson.hpp"

#include "vendor/Soup/soup/os.hpp"

[[nodiscard]] std::filesystem::path getStringStreamPathForRead(lua_State* L, int idx);

static int encode(lua_State* L) {
	auto root = checkJson(L, 1);
	if (lua_istrue(L, 2))
	{
		pluto_pushstring(L, root->encodePretty());
	}
	else
	{
		pluto_pushstring(L, root->encode());
	}
	return 1;
}

static int decode(lua_State* L)
{
	if (auto root = soup::json::decode(pluto_checkstring(L, 1)))
	{
		pushFromJson(L, *root);
		return 1;
	}
	return 0;
}

static int decodefile (lua_State* L)
{
	auto path = getStringStreamPathForRead(L, 1);
	size_t len;
	if (auto data = soup::os::createFileMapping(path, len))
	{
		auto c = (const char*)data;
		if (auto root = soup::json::decode(c))
		{
			pushFromJson(L, *root);
			soup::os::destroyFileMapping(data, len);
			return 1;
		}
		else
		{
			soup::os::destroyFileMapping(data, len);
		}
	}
	return 0;
}

static const luaL_Reg funcs[] = {
	{"encode", encode},
	{"decode", decode},
	{"decodefile", decodefile},
	{nullptr, nullptr}
};

LUAMOD_API int luaopen_json(lua_State* L)
{
	luaL_newlib(L, funcs);
	lua_pushlightuserdata(L, reinterpret_cast<void*>(static_cast<uintptr_t>('PJNL')));
	lua_setfield(L, -2, "null");
	return 1;
}
const Pluto::PreloadedLibrary Pluto::preloaded_json{ "json", funcs, &luaopen_json };
