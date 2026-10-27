export interface ILibplutoModule {
	cwrap: <T extends (...args: any[]) => any>(
		ident: string,
		returnType: string,
		argTypes: string[]
	) => T;

	getValue: (ptr: number, type: string) => number;
	setValue: (ptr: number, value: number, type: string) => void;

	FS: {
		open: (path: string, flags: string) => number | { fd: number };
		write: (
			stream: number | { fd: number },
			buffer: Uint8Array,
			offset: number,
			length: number,
			position: number
			) => number;
		close: (stream: number | { fd: number }) => void;

		writeFile: (path: string, data: Uint8Array | string, opts?: { encoding?: string; flags?: string }) => void;
		readFile?: (path: string, opts?: { encoding?: 'utf8' | 'binary' }) => string | Uint8Array;
		unlink?: (path: string) => void;
	};
}

export interface ILibplutoWrapper {
	readonly mod: ILibplutoModule;

	readonly LUA_OK: 0;
	readonly LUA_YIELD: 1;
	readonly LUA_ERRRUN: 2;
	readonly LUA_ERRSYNTAX: 3;
	readonly LUA_ERRMEM: 4;
	readonly LUA_ERRERR: 5;
	readonly LUA_ERRFILE: 6;

	readonly LUA_TNONE: -1;
	readonly LUA_TNIL: 0;
	readonly LUA_TBOOLEAN: 1;
	readonly LUA_TLIGHTUSERDATA: 2;
	readonly LUA_TNUMBER: 3;
	readonly LUA_TSTRING: 4;
	readonly LUA_TTABLE: 5;
	readonly LUA_TFUNCTION: 6;
	readonly LUA_TUSERDATA: 7;
	readonly LUA_TTHREAD: 8;
	readonly LUA_NUMTYPES: 9;

	readonly LUA_MULTRET: -1;
	readonly LUA_MINSTACK: 20;
	readonly LUA_REGISTRYINDEX: -1073742823;
	readonly LUA_RIDX_GLOBALS: 2;
	readonly LUA_RIDX_MAINTHREAD: 3;

	readonly LUA_REFNIL: -1;
	readonly LUA_NOREF: -2;

	readonly LUA_GNAME: "_G";
	readonly LUA_LOADED_TABLE: "_LOADED";
	readonly LUA_PRELOAD_TABLE: "_PRELOAD";
	readonly LUA_FILEHANDLE: "FILE*";

	readonly LUA_OPADD: 0;
	readonly LUA_OPSUB: 1;
	readonly LUA_OPMUL: 2;
	readonly LUA_OPMOD: 3;
	readonly LUA_OPPOW: 4;
	readonly LUA_OPDIV: 5;
	readonly LUA_OPIDIV: 6;
	readonly LUA_OPBAND: 7;
	readonly LUA_OPBOR: 8;
	readonly LUA_OPBXOR: 9;
	readonly LUA_OPSHL: 10;
	readonly LUA_OPSHR: 11;
	readonly LUA_OPUNM: 12;
	readonly LUA_OPBNOT: 13;

	readonly LUA_OPEQ: 0;
	readonly LUA_OPLT: 1;
	readonly LUA_OPLE: 2;

	readonly LUA_GCSTOP: 0;
	readonly LUA_GCRESTART: 1;
	readonly LUA_GCCOLLECT: 2;
	readonly LUA_GCCOUNT: 3;
	readonly LUA_GCCOUNTB: 4;
	readonly LUA_GCSTEP: 5;
	readonly LUA_GCISRUNNING: 6;
	readonly LUA_GCGEN: 7;
	readonly LUA_GCINC: 8;
	readonly LUA_GCPARAM: 9;

	readonly malloc: (size: number) => number;
	readonly free: (ptr: number) => void;

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
	readonly lua_pcallk: (
		L: number,
		nargs: number,
		nresults: number,
		errfunc: number,
		ctx: number,
		k: number
	) => number;
	readonly lua_getglobal: (L: number, name: string) => number;
	readonly lua_type: (L: number, idx: number) => number;
	readonly lua_typename: (L: number, tp: number) => string;
	readonly lua_pushnil: (L: number) => void;
	readonly lua_pushstring: (L: number, str: string) => number;
	readonly lua_pushlstring: (L: number, str: Uint8Array, len: number) => number;
	readonly lua_pushinteger: (L: number, n: bigint) => void;
	readonly lua_pushnumber: (L: number, n: number) => void;
	readonly lua_pushboolean: (L: number, b: number) => void;
	readonly lua_pushthread: (L: number) => number;
	readonly lua_istrue: (L: number, idx: number) => number;
	readonly lua_isinteger: (L: number, idx: number) => number;
	readonly lua_isnumber: (L: number, idx: number) => number;
	readonly lua_isstring: (L: number, idx: number) => number;
	readonly lua_isuserdata: (L: number, idx: number) => number;
	readonly lua_iscfunction: (L: number, idx: number) => number;
	readonly lua_isyieldable: (L: number) => number;
	readonly lua_toboolean: (L: number, idx: number) => number;
	readonly lua_tolstring: (L: number, idx: number, lenPtr: number) => string;
	readonly lua_tointegerx: (L: number, idx: number, isnum: number) => bigint;
	readonly lua_tonumberx: (L: number, idx: number, isnum: number) => number;
	readonly lua_version: (L: number) => number;
	readonly lua_settop: (L: number, idx: number) => void;
	readonly lua_absindex: (L: number, idx: number) => number;
	readonly lua_checkstack: (L: number, n: number) => number;
	readonly lua_rotate: (L: number, idx: number, n: number) => void;
	readonly lua_copy: (L: number, fromidx: number, toidx: number) => void;
	readonly lua_arith: (L: number, op: number) => void;
	readonly lua_rawequal: (L: number, idx1: number, idx2: number) => number;
	readonly lua_compare: (L: number, idx1: number, idx2: number, op: number) => number;
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
	readonly lua_concat: (L: number, n: number) => void;
	readonly lua_len: (L: number, idx: number) => void;
	readonly lua_stringtonumber: (L: number, str: string) => number;
	readonly luaL_ref: (L: number, t: number) => number;
	readonly lua_rawgeti: (L: number, t: number, ref: bigint) => number;
	readonly lua_rawseti: (L: number, t: number, n: bigint) => void;
	readonly lua_rawget: (L: number, idx: number) => number;
	readonly lua_rawset: (L: number, idx: number) => void;
	readonly luaL_unref: (L: number, t: number, ref: number) => void;
	readonly lua_next: (L: number, idx: number) => number;
	readonly lua_createtable: (L: number, narr: number, nrec: number) => void;
	readonly lua_gettable: (L: number, idx: number) => number;
	readonly lua_settable: (L: number, idx: number) => void;
	readonly lua_getfield: (L: number, idx: number, k: string) => number;
	readonly lua_setfield: (L: number, idx: number, k: string) => void;
	readonly lua_geti: (L: number, idx: number, n: bigint) => number;
	readonly lua_seti: (L: number, idx: number, n: bigint) => void;
	readonly lua_getmetatable: (L: number, objindex: number) => number;
	readonly lua_setmetatable: (L: number, objindex: number) => number;
	readonly lua_toclose: (L: number, idx: number) => void;
	readonly lua_closeslot: (L: number, idx: number) => void;
	readonly luaL_loadfilex: (
		L: number,
		filename: string,
		mode: number
	) => number;
	readonly lua_close: (L: number) => void;

	readonly luaL_getmetafield: (L: number, obj: number, e: string) => number;
	readonly luaL_callmeta: (L: number, obj: number, e: string) => number;
	readonly luaL_tolstring: (L: number, idx: number, lenPtr: number) => string;
	readonly luaL_argerror: (L: number, arg: number, extramsg: string) => number;
	readonly luaL_typeerror: (L: number, arg: number, tname: string) => number;
	readonly luaL_checklstring: (L: number, arg: number, lenPtr: number) => string;
	readonly luaL_optlstring: (L: number, arg: number, def: string | null, lenPtr: number) => string;
	readonly luaL_checknumber: (L: number, arg: number) => number;
	readonly luaL_optnumber: (L: number, arg: number, def: number) => number;
	readonly luaL_checkinteger: (L: number, arg: number) => bigint;
	readonly luaL_optinteger: (L: number, arg: number, def: bigint) => bigint;
	readonly luaL_checkstack: (L: number, sz: number, msg: string | null) => void;
	readonly luaL_checktype: (L: number, arg: number, t: number) => void;
	readonly luaL_checkany: (L: number, arg: number) => void;
	readonly luaL_newmetatable: (L: number, tname: string) => number;
	readonly luaL_setmetatable: (L: number, tname: string) => void;
	readonly luaL_testudata: (L: number, ud: number, tname: string) => number;
	readonly luaL_checkudata: (L: number, ud: number, tname: string) => number;
	readonly luaL_where: (L: number, lvl: number) => void;
	readonly luaL_fileresult: (L: number, stat: number, fname: string | null) => number;
	readonly luaL_execresult: (L: number, stat: number) => number;
	readonly luaL_makeseed: (L: number) => number;
	readonly luaL_len: (L: number, idx: number) => bigint;
	readonly luaL_gsub: (L: number, s: string, p: string, r: string) => string;
	readonly luaL_getsubtable: (L: number, idx: number, fname: string) => number;
	readonly luaL_traceback: (L: number, L1: number, msg: string | null, level: number) => void;
	readonly luaL_requiref: (L: number, modname: string, openf: number, glb: number) => void;

	readonly luaL_checkstring: (L: number, n: number) => string;
	readonly luaL_optstring: (L: number, n: number, d: string) => string;
	readonly luaL_typename: (L: number, i: number) => string;
	readonly luaL_getmetatable: (L: number, n: string) => number;
	readonly luaL_loadfile: (L: number, filename: string) => number;
	readonly luaL_dofile: (L: number, filename: string) => number;
	readonly luaL_dostring: (L: number, s: string) => number;
	readonly luaL_pushfail: (L: number) => void;
	readonly luaL_argcheck: (L: number, cond: boolean | number, arg: number, extramsg: string) => void;
	readonly luaL_argexpected: (L: number, cond: boolean | number, arg: number, tname: string) => void;

	readonly lua_call: (L: number, nargs: number, nresults: number) => void;
	readonly lua_pcall: (L: number, nargs: number, nresults: number, errfunc: number) => number;
	readonly lua_pop: (L: number, n: number) => void;
	readonly lua_newtable: (L: number) => void;
	readonly lua_pushglobaltable: (L: number) => void;
	readonly lua_upvalueindex: (i: number) => number;
	readonly lua_tonumber: (L: number, idx: number) => number;
	readonly lua_tointeger: (L: number, idx: number) => bigint;
	readonly lua_tostring: (L: number, idx: number) => string;
	readonly lua_isnil: (L: number, idx: number) => 0 | 1;
	readonly lua_isboolean: (L: number, idx: number) => 0 | 1;
	readonly lua_istable: (L: number, idx: number) => 0 | 1;
	readonly lua_isfunction: (L: number, idx: number) => 0 | 1;
	readonly lua_isnone: (L: number, idx: number) => 0 | 1;
	readonly lua_isnoneornil: (L: number, idx: number) => 0 | 1;
}

declare function createWrapper(emscripten_module: ILibplutoModule): ILibplutoWrapper;
declare global {
	interface Window {
		libplutoWrapper: typeof createWrapper;
	}
}
export default createWrapper;
