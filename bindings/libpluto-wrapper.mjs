const createWrapper = function(mod)
{
	const w = {};

	w.LUA_OK = 0;
	w.LUA_YIELD = 1;
	w.LUA_ERRRUN = 2;
	w.LUA_ERRSYNTAX = 3;
	w.LUA_ERRMEM = 4;
	w.LUA_ERRERR = 5;

	w.LUA_TNONE = -1;
	w.LUA_TNIL = 0;
	w.LUA_TBOOLEAN = 1;
	w.LUA_TLIGHTUSERDATA = 2;
	w.LUA_TNUMBER = 3;
	w.LUA_TSTRING = 4;
	w.LUA_TTABLE = 5;
	w.LUA_TFUNCTION = 6;
	w.LUA_TUSERDATA = 7;
	w.LUA_TTHREAD = 8;
	w.LUA_NUMTYPES = 9;

	w.LUA_MULTRET = -1;
	w.LUA_MINSTACK = 20;
	const INT_MAX = 2147483647;
	w.LUA_REGISTRYINDEX = (-(Math.trunc(INT_MAX/2) + 1000));
	w.LUA_RIDX_GLOBALS = 2;
	w.LUA_RIDX_MAINTHREAD = 3;

	w.LUA_REFNIL = -1;
	w.LUA_NOREF = -2;

	w.LUA_OPADD = 0;
	w.LUA_OPSUB = 1;
	w.LUA_OPMUL = 2;
	w.LUA_OPMOD = 3;
	w.LUA_OPPOW = 4;
	w.LUA_OPDIV = 5;
	w.LUA_OPIDIV = 6;
	w.LUA_OPBAND = 7;
	w.LUA_OPBOR = 8;
	w.LUA_OPBXOR = 9;
	w.LUA_OPSHL = 10;
	w.LUA_OPSHR = 11;
	w.LUA_OPUNM = 12;
	w.LUA_OPBNOT = 13;

	w.LUA_OPEQ = 0;
	w.LUA_OPLT = 1;
	w.LUA_OPLE = 2;

	w.LUA_GCSTOP = 0;
	w.LUA_GCRESTART = 1;
	w.LUA_GCCOLLECT = 2;
	w.LUA_GCCOUNT = 3;
	w.LUA_GCCOUNTB = 4;
	w.LUA_GCSTEP = 5;
	w.LUA_GCISRUNNING = 6;
	w.LUA_GCGEN = 7;
	w.LUA_GCINC = 8;
	w.LUA_GCPARAM = 9;

	w.malloc = mod.cwrap("malloc", "int", ["int"]);
	w.luaL_newstate = mod.cwrap("luaL_newstate", "int", []);
	w.luaL_openselectedlibs = mod.cwrap("luaL_openselectedlibs", "void", ["int", "int", "int"]);
	w.luaL_openlibs = (L) => w.luaL_openselectedlibs(L, 1023n, 0xffffffffffffffffn);
	w.luaL_loadstring = mod.cwrap("luaL_loadstring", "int", ["int", "string"]);
	w.luaL_loadbufferx = mod.cwrap("luaL_loadbufferx", "int", ["int", "array", "int", "int", "int"]);
	w.lua_callk = mod.cwrap("lua_callk", "void", ["int", "int", "int", "int", "int"]);
	w.lua_pcallk = mod.cwrap("lua_pcallk", "int", ["int", "int", "int", "int", "int", "int"]);
	w.lua_getglobal = mod.cwrap("lua_getglobal", "int", ["int", "string"]);
	w.lua_type = mod.cwrap("lua_type", "int", ["int", "int"]);
	w.lua_typename = mod.cwrap("lua_typename", "string", ["int", "int"]);
	w.lua_pushnil = mod.cwrap("lua_pushnil", "void", ["int"]);
	w.lua_pushstring = mod.cwrap("lua_pushstring", "int", ["int", "string"]);
	w.lua_pushlstring = mod.cwrap("lua_pushlstring", "int", ["int", "array", "int"]);
	w.lua_pushinteger = mod.cwrap("lua_pushinteger", "void", ["int", "int"]);
	w.lua_pushnumber = mod.cwrap("lua_pushnumber", "void", ["int", "number"]);
	w.lua_pushboolean = mod.cwrap("lua_pushboolean", "void", ["int", "int"]);
	w.lua_pushthread = mod.cwrap("lua_pushthread", "int", ["int"]);
	w.lua_istrue = mod.cwrap("lua_istrue", "int", ["int", "int"]);
	w.lua_isinteger = mod.cwrap("lua_isinteger", "int", ["int", "int"]);
	w.lua_isnumber = mod.cwrap("lua_isnumber", "int", ["int", "int"]);
	w.lua_isstring = mod.cwrap("lua_isstring", "int", ["int", "int"]);
	w.lua_isuserdata = mod.cwrap("lua_isuserdata", "int", ["int", "int"]);
	w.lua_iscfunction = mod.cwrap("lua_iscfunction", "int", ["int", "int"]);
	w.lua_isyieldable = mod.cwrap("lua_isyieldable", "int", ["int"]);
	w.lua_toboolean = mod.cwrap("lua_toboolean", "int", ["int", "int"]);
	w.lua_tolstring = mod.cwrap("lua_tolstring", "string", ["int", "int", "int"]);
	w.lua_tointegerx = mod.cwrap("lua_tointegerx", "int", ["int", "int", "int"]);
	w.lua_tonumberx = mod.cwrap("lua_tonumberx", "number", ["int", "int", "int"]);
	w.lua_version = mod.cwrap("lua_version", "number", ["int"]);
	w.lua_settop = mod.cwrap("lua_settop", "void", ["int", "int"]);
	w.lua_absindex = mod.cwrap("lua_absindex", "int", ["int", "int"]);
	w.lua_checkstack = mod.cwrap("lua_checkstack", "int", ["int", "int"]);
	w.lua_rotate = mod.cwrap("lua_rotate", "void", ["int", "int", "int"]);
	w.lua_copy = mod.cwrap("lua_copy", "void", ["int", "int", "int"]);
	w.lua_arith = mod.cwrap("lua_arith", "void", ["int", "int"]);
	w.lua_rawequal = mod.cwrap("lua_rawequal", "int", ["int", "int", "int"]);
	w.lua_compare = mod.cwrap("lua_compare", "int", ["int", "int", "int", "int"]);
	w.lua_setglobal = mod.cwrap("lua_setglobal", "void", ["int", "string"]);
	w.lua_newthread = mod.cwrap("lua_newthread", "int", ["int"]);
	w.lua_xmove = mod.cwrap("lua_xmove", "void", ["int", "int", "int"]);
	w.lua_closethread = mod.cwrap("lua_closethread", "int", ["int", "int"]);
	w.lua_status = mod.cwrap("lua_status", "int", ["int"]);
	w.lua_resume = mod.cwrap("lua_resume", "int", ["int", "int", "int", "int"]);
	w.lua_gettop = mod.cwrap("lua_gettop", "int", ["int"]);
	w.lua_pushvalue = mod.cwrap("lua_pushvalue", "void", ["int", "int"]);
	w.lua_concat = mod.cwrap("lua_concat", "void", ["int", "int"]);
	w.lua_len = mod.cwrap("lua_len", "void", ["int", "int"]);
	w.lua_stringtonumber = mod.cwrap("lua_stringtonumber", "int", ["int", "string"]);
	w.luaL_ref = mod.cwrap("luaL_ref", "int", ["int", "int"]);
	w.lua_rawgeti = mod.cwrap("lua_rawgeti", "int", ["int", "int", "int"]);
	w.lua_rawseti = mod.cwrap("lua_rawseti", "void", ["int", "int", "int"]);
	w.lua_rawget = mod.cwrap("lua_rawget", "int", ["int", "int"]);
	w.lua_rawset = mod.cwrap("lua_rawset", "void", ["int", "int"]);
	w.luaL_unref = mod.cwrap("luaL_unref", "void", ["int", "int", "int"]);
	w.lua_next = mod.cwrap("lua_next", "int", ["int", "int"]);
	w.lua_createtable = mod.cwrap("lua_createtable", "void", ["int", "int", "int"]);
	w.lua_gettable = mod.cwrap("lua_gettable", "int", ["int", "int"]);
	w.lua_settable = mod.cwrap("lua_settable", "void", ["int", "int"]);
	w.lua_getfield = mod.cwrap("lua_getfield", "int", ["int", "int", "string"]);
	w.lua_setfield = mod.cwrap("lua_setfield", "void", ["int", "int", "string"]);
	w.lua_geti = mod.cwrap("lua_geti", "int", ["int", "int", "int"]);
	w.lua_seti = mod.cwrap("lua_seti", "void", ["int", "int", "int"]);
	w.lua_getmetatable = mod.cwrap("lua_getmetatable", "int", ["int", "int"]);
	w.lua_setmetatable = mod.cwrap("lua_setmetatable", "int", ["int", "int"]);
	w.lua_toclose = mod.cwrap("lua_toclose", "void", ["int", "int"]);
	w.lua_closeslot = mod.cwrap("lua_closeslot", "void", ["int", "int"]);
	w.luaL_loadfilex = mod.cwrap("luaL_loadfilex", "int", ["int", "string", "int"]);
	w.lua_close = mod.cwrap("lua_close", "void", ["int"]);

	w.lua_call = (L, nargs, nresults) => w.lua_callk(L, nargs, nresults, 0, 0);
	w.lua_pcall = (L, nargs, nresults, errfunc) => w.lua_pcallk(L, nargs, nresults, errfunc, 0, 0);
	w.lua_pop = (L, n) => w.lua_settop(L, -(n)-1);
	w.lua_newtable = (L) => w.lua_createtable(L, 0, 0);
	w.lua_pushglobaltable = (L) => w.lua_rawgeti(L, w.LUA_REGISTRYINDEX, BigInt(w.LUA_RIDX_GLOBALS));
	w.lua_upvalueindex = (i) => w.LUA_REGISTRYINDEX - i;
	w.lua_tonumber = (L, i) => w.lua_tonumberx(L, i, 0);
	w.lua_tointeger = (L, i) => w.lua_tointegerx(L, i, 0);
	w.lua_tostring = (L, i) => w.lua_tolstring(L, i, 0);
	w.lua_isnil = (L, n) => w.lua_type(L, n) === w.LUA_TNIL ? 1 : 0;
	w.lua_isboolean = (L, n) => w.lua_type(L, n) === w.LUA_TBOOLEAN ? 1 : 0;
	w.lua_istable = (L, n) => w.lua_type(L, n) === w.LUA_TTABLE ? 1 : 0;
	w.lua_isfunction = (L, n) => w.lua_type(L, n) === w.LUA_TFUNCTION ? 1 : 0;
	w.lua_isnone = (L, n) => w.lua_type(L, n) === w.LUA_TNONE ? 1 : 0;
	w.lua_isnoneornil = (L, n) => w.lua_type(L, n) <= 0 ? 1 : 0;

	return w;
};

if (typeof window !== "undefined")
{
	window.libplutoWrapper = createWrapper;
}

export default createWrapper;
