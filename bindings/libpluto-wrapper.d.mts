/// <reference types="emscripten" />

interface IWrapper {
	readonly LUA_OK: 0;
	readonly LUA_YIELD: 1;
	readonly LUA_ERRRUN: 2;
	readonly LUA_ERRSYNTAX: 3;
	readonly LUA_ERRMEM: 4;
	readonly LUA_ERRERR: 5;

	readonly LUA_TNIL: 0;
	readonly LUA_TBOOLEAN: 1;
	readonly LUA_TLIGHTUSERDATA: 2;
	readonly LUA_TNUMBER: 3;
	readonly LUA_TSTRING: 4;
	readonly LUA_TTABLE: 5;
	readonly LUA_TFUNCTION: 6;
	readonly LUA_TUSERDATA: 7;
	readonly LUA_TTHREAD: 8;

	readonly malloc: (size: number) => number;
	readonly luaL_newstate: () => number;
	readonly luaL_openselectedlibs: (L: number, load: bigint, preload: bigint) => void;
	readonly luaL_openlibs: (L: number) => void;
	readonly luaL_loadstring: (L: number, str: string) => number;
	readonly luaL_loadbufferx: (
	  L: number,
	  buf: Uint8Array,
	  size: number,
	  name: number,
	  mode: number
	) => number;
	readonly lua_callk: (
	  L: number,
	  nargs: number,
	  nresults: number,
	  ctx: number,
	  k: number
	) => void;
	readonly lua_getglobal: (L: number, name: string) => number;
	readonly lua_type: (L: number, idx: number) => number;
	readonly lua_pushnil: (L: number) => void;
	readonly lua_pushstring: (L: number, str: string) => number;
	readonly lua_pushlstring: (L: number, str: Uint8Array, len: number) => number;
	readonly lua_pushinteger: (L: number, n: bigint) => void;
	readonly lua_pushnumber: (L: number, n: number) => void;
	readonly lua_pushboolean: (L: number, b: number) => void;
	readonly lua_istrue: (L: number, idx: number) => number;
	readonly lua_isinteger: (L: number, idx: number) => number;
	readonly lua_tolstring: (L: number, idx: number, lenPtr: number) => string;
	readonly lua_tointegerx: (L: number, idx: number, isnum: number) => bigint;
	readonly lua_tonumberx: (L: number, idx: number, isnum: number) => number;
	readonly lua_settop: (L: number, idx: number) => void;
	readonly lua_setglobal: (L: number, name: string) => void;
	readonly lua_newthread: (L: number) => number;
	readonly lua_xmove: (from: number, to: number, n: number) => void;
	readonly lua_closethread: (L: number, from: number) => number;
	readonly lua_status: (L: number) => number;
	readonly lua_resume: (
	  L: number,
	  from: number,
	  nargs: number,
	  nres: number
	) => number;
	readonly lua_gettop: (L: number) => number;
	readonly lua_pushvalue: (L: number, idx: number) => void;
	readonly luaL_ref: (L: number, t: number) => number;
	readonly lua_rawgeti: (L: number, t: number, ref: bigint) => number;
	readonly luaL_unref: (L: number, t: number, ref: number) => void;
	readonly lua_next: (L: number) => number;
	readonly lua_createtable: (L: number, narr: number, nrec: number) => void;
	readonly lua_settable: (L: number, idx: number) => void;
	readonly luaL_loadfilex: (
	  L: number,
	  filename: string,
	  mode: number
	) => number;
	readonly lua_close: (L: number) => void;

	readonly lua_pop: (L: number, n: number) => void;
}

declare function createWrapper(emscripten_module: EmscriptenModule): IWrapper;
export default createWrapper;
