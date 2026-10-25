export default function(mod)
{
	const w = {};

	w.LUA_OK = 0;
	w.LUA_YIELD = 1;
	w.LUA_ERRRUN = 2;
	w.LUA_ERRSYNTAX = 3;
	w.LUA_ERRMEM = 4;
	w.LUA_ERRERR = 5;

	w.malloc = mod.cwrap("malloc", "int", ["int"]);
	w.luaL_newstate = mod.cwrap("luaL_newstate", "int", []);
	w.luaL_openselectedlibs = mod.cwrap("luaL_openselectedlibs", "int", ["int", "int", "int"]);
	w.luaL_openlibs = (L) => w.luaL_openselectedlibs(L, 1023n, 0xffffffffffffffffn);
	w.luaL_loadstring = mod.cwrap("luaL_loadstring", "int", ["int", "string"]);
	w.luaL_loadbufferx = mod.cwrap("luaL_loadbufferx", "int", ["int", "array", "int", "int", "int"]);
	w.lua_callk = mod.cwrap("lua_callk", "void", ["int", "int", "int", "int", "int"]);
	w.lua_getglobal = mod.cwrap("lua_getglobal", "void", ["int", "string"]);
	w.lua_type = mod.cwrap("lua_type", "int", ["int", "int"]);
	w.lua_pushnil = mod.cwrap("lua_pushnil", "void", ["int"]);
	w.lua_pushstring = mod.cwrap("lua_pushstring", "void", ["int", "string"]);
	w.lua_pushlstring = mod.cwrap("lua_pushlstring", "void", ["int", "array", "int"]);
	w.lua_pushinteger = mod.cwrap("lua_pushinteger", "void", ["int", "int"]);
	w.lua_pushnumber = mod.cwrap("lua_pushnumber", "void", ["int", "number"]);
	w.lua_pushboolean = mod.cwrap("lua_pushboolean", "void", ["int", "int"]);
	w.lua_istrue = mod.cwrap("lua_istrue", "int", ["int", "int"]);
	w.lua_isinteger = mod.cwrap("lua_isinteger", "int", ["int", "int"]);
	w.lua_tolstring = mod.cwrap("lua_tolstring", "string", ["int", "int", "int"]);
	w.lua_tointegerx = mod.cwrap("lua_tointegerx", "int", ["int", "int", "int"]);
	w.lua_tonumberx = mod.cwrap("lua_tonumberx", "number", ["int", "int", "int"]);
	w.lua_settop = mod.cwrap("lua_settop", "void", ["int", "int"]);
	w.lua_setglobal = mod.cwrap("lua_setglobal", "void", ["int", "string"]);
	w.lua_newthread = mod.cwrap("lua_newthread", "int", ["int"]);
	w.lua_xmove = mod.cwrap("lua_xmove", "void", ["int", "int", "int"]);
	w.lua_closethread = mod.cwrap("lua_closethread", "void", ["int", "int"]);
	w.lua_status = mod.cwrap("lua_status", "int", ["int"]);
	w.lua_resume = mod.cwrap("lua_resume", "int", ["int", "int", "int", "int"]);
	w.lua_gettop = mod.cwrap("lua_gettop", "int", ["int"]);
	w.lua_pushvalue = mod.cwrap("lua_pushvalue", "int", ["int", "int"]);
	w.luaL_ref = mod.cwrap("luaL_ref", "int", ["int", "int"]);
	w.lua_rawgeti = mod.cwrap("lua_rawgeti", "void", ["int", "int", "int"]);
	w.luaL_unref = mod.cwrap("luaL_unref", "int", ["int", "int", "int"]);
	w.lua_next = mod.cwrap("lua_next", "int", ["int"]);
	w.lua_createtable = mod.cwrap("lua_createtable", "void", ["int", "int", "int"]);
	w.lua_settable = mod.cwrap("lua_settable", "void", ["int", "int"]);
	w.luaL_loadfilex = mod.cwrap("luaL_loadfilex", "int", ["int", "string", "int"]);
	w.lua_close = mod.cwrap("lua_close", "void", ["int"]);

	w.lua_pop = (L, n) => w.lua_settop(L, -(n)-1);

	return w;
};
