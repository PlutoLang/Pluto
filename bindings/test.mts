import { strict as assert } from "node:assert";

// @ts-expect-error no declaration file
import libpluto from "../libpluto.js";
import wrapper from "./libpluto-wrapper.mjs";

// This file performs various unit tests to make sure the typings in libpluto-wrapper.d.mts are consistent with what gets marshalled by Emscripten (argument types & return types).
// As well as that the constants provided match the headers and therefore have the expected functionality.
// Otherwise this file is NOT a C API functionality test.

const mod = await (await libpluto)();
const {
	LUA_TNIL, LUA_TBOOLEAN, LUA_TNUMBER, LUA_TSTRING, LUA_TTABLE, LUA_TFUNCTION, LUA_ERRRUN,
	LUA_OPADD, LUA_OPLT,
	luaL_newstate, luaL_openlibs, luaL_loadstring, lua_close,
	lua_pushnil, lua_pushstring, lua_pushlstring, lua_pushinteger, lua_pushnumber, lua_pushboolean,
	lua_type, lua_istrue, lua_isinteger, lua_tointegerx, lua_tonumberx, lua_tolstring,
	lua_gettop, lua_setglobal, lua_getglobal,
	lua_typename, lua_arith, lua_compare,
	lua_createtable, lua_getfield, lua_setfield, lua_geti, lua_seti, lua_next,
	lua_stringtonumber, lua_pcall, lua_pushglobaltable,
	lua_isnil, lua_isnoneornil,
	lua_rawgeti, lua_rawget, lua_rawset, lua_rawequal, lua_pushthread,
	lua_newthread, lua_resume, malloc, free,
	luaL_ref, luaL_unref,
	LUA_TTHREAD, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS, LUA_RIDX_MAINTHREAD,
	LUA_REFNIL, LUA_NOREF,
	LUA_OK, LUA_ERRFILE, lua_pop,
	luaL_checkinteger, luaL_optinteger, luaL_checknumber, luaL_optnumber,
	luaL_checkstring, luaL_optstring, luaL_gsub, luaL_tolstring, luaL_len, luaL_getmetafield,
	luaL_getsubtable, luaL_newmetatable, luaL_loadfilex, luaL_dostring,
} = wrapper(mod);

let failures = 0;
function test(name: string, fn: (L: number) => void) {
	const L = luaL_newstate();
	try {
		fn(L);
		console.log(`PASS ${name}`);
	} catch (e) {
		failures++;
		console.error(`FAIL ${name}: ${(e as Error).message}`);
	} finally {
		lua_close(L);
	}
}

// Strict equality check. Being generic in T lets TypeScript flag comparisons
// between incompatible types (e.g. bigint vs number) at compile time.
function expectEqual<T>(actual: T, expected: T, msg?: string): void {
	assert.equal(actual, expected, msg);
}

test("lua_pushnumber pushes a float that reads back", L => {
	lua_pushnumber(L, 3.5);
	expectEqual(lua_type(L, -1), LUA_TNUMBER);
	expectEqual(lua_tonumberx(L, -1, 0), 3.5);
	expectEqual(lua_isinteger(L, -1), 0);
});

test("lua_pushnumber keeps integral-valued numbers as floats", L => {
	lua_pushnumber(L, 2);
	expectEqual(lua_isinteger(L, -1), 0);
	expectEqual(lua_tonumberx(L, -1, 0), 2);
});

test("lua_pushinteger pushes an integer", L => {
	lua_pushinteger(L, 42n);
	expectEqual(lua_type(L, -1), LUA_TNUMBER);
	expectEqual(lua_isinteger(L, -1), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 42n);
	expectEqual(lua_tonumberx(L, -1, 0), 42);
});

test("lua_pushnil pushes nil", L => {
	lua_pushnil(L);
	expectEqual(lua_type(L, -1), LUA_TNIL);
	expectEqual(lua_gettop(L), 1);
});

test("lua_pushboolean true is a boolean and truthy", L => {
	lua_pushboolean(L, 1);
	expectEqual(lua_type(L, -1), LUA_TBOOLEAN);
	expectEqual(lua_istrue(L, -1), 1);
});

test("lua_pushboolean false is a boolean and falsy", L => {
	lua_pushboolean(L, 0);
	expectEqual(lua_type(L, -1), LUA_TBOOLEAN);
	expectEqual(lua_istrue(L, -1), 0);
});

test("lua_pushstring pushes a string", L => {
	lua_pushstring(L, "hello");
	expectEqual(lua_type(L, -1), LUA_TSTRING);
	expectEqual(lua_tolstring(L, -1, 0), "hello");
});

test("lua_pushlstring pushes exactly len bytes", L => {
	const bytes = new TextEncoder().encode("hello");
	lua_pushlstring(L, bytes, 2);
	expectEqual(lua_type(L, -1), LUA_TSTRING);
	expectEqual(lua_tolstring(L, -1, 0), "he");
});

test("lua_tointegerx returns a bigint", L => {
	lua_pushnumber(L, 3.0);
	expectEqual(lua_tointegerx(L, -1, 0), 3n);
});

test("lua_gettop counts pushed values", L => {
	lua_pushinteger(L, 1n);
	lua_pushnil(L);
	lua_pushboolean(L, 1);
	expectEqual(lua_gettop(L), 3);
});

test("lua_setglobal / lua_getglobal round-trip a number", L => {
	lua_pushnumber(L, 1.25);
	lua_setglobal(L, "answer");
	expectEqual(lua_gettop(L), 0, "setglobal should pop the value");

	expectEqual(lua_getglobal(L, "answer"), LUA_TNUMBER);
	expectEqual(lua_tonumberx(L, -1, 0), 1.25);
});

test("lua_typename names basic types", L => {
	expectEqual(lua_typename(L, LUA_TTABLE), "table");
	expectEqual(lua_typename(L, LUA_TFUNCTION), "function");
	expectEqual(lua_typename(L, LUA_TNIL), "nil");
});

test("lua_arith with LUA_OPADD adds the two top values", L => {
	lua_pushinteger(L, 2n);
	lua_pushinteger(L, 3n);
	lua_arith(L, LUA_OPADD);
	expectEqual(lua_gettop(L), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 5n);
});

test("lua_compare with LUA_OPLT reports less-than", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	expectEqual(lua_compare(L, 1, 2, LUA_OPLT), 1);
	expectEqual(lua_compare(L, 2, 1, LUA_OPLT), 0);
});

test("lua_setfield / lua_getfield round-trip a table field", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 5n);
	lua_setfield(L, -2, "x");
	expectEqual(lua_gettop(L), 1, "setfield should pop the value");
	expectEqual(lua_getfield(L, -1, "x"), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 5n);
});

test("lua_seti / lua_geti round-trip an array element", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 7n);
	lua_seti(L, -2, 1n);
	expectEqual(lua_geti(L, -1, 1n), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 7n);
});

test("lua_next returns 0 and pushes nothing for an empty table", L => {
	lua_createtable(L, 0, 0);
	lua_pushnil(L);
	expectEqual(lua_next(L, -2), 0);
	expectEqual(lua_gettop(L), 1, "only the table should remain");
});

test("lua_stringtonumber returns size+1 for a parsed number", L => {
	expectEqual(lua_stringtonumber(L, "42"), 3);
});

test("lua_pcall returns LUA_ERRRUN for a raised error", L => {
	luaL_openlibs(L);
	luaL_loadstring(L, "error('boom')");
	expectEqual(lua_pcall(L, 0, 0, 0), LUA_ERRRUN);
});

test("lua_isnil and lua_isnoneornil work on nil and none", L => {
	lua_pushnil(L);
	expectEqual(lua_isnil(L, -1), 1);
	expectEqual(lua_isnoneornil(L, 5), 1);
	expectEqual(lua_isnoneornil(L, -1), 1);
	lua_pushinteger(L, 1n);
	expectEqual(lua_isnil(L, -1), 0);
});

test("LUA_REGISTRYINDEX is a large negative pseudo-index", () => {
	assert.ok(LUA_REGISTRYINDEX < -1_000_000, `got ${LUA_REGISTRYINDEX}`);
});

test("registry holds the globals table at LUA_RIDX_GLOBALS", L => {
	lua_pushglobaltable(L);
	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(LUA_RIDX_GLOBALS)), LUA_TTABLE);
	expectEqual(lua_rawequal(L, -1, -2), 1, "registry entry should be the globals table");
});

test("registry holds the main thread at LUA_RIDX_MAINTHREAD", L => {
	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(LUA_RIDX_MAINTHREAD)), LUA_TTHREAD);
	expectEqual(lua_pushthread(L), 1, "pushthread should report the main thread");
	expectEqual(lua_rawequal(L, -1, -2), 1, "registry entry should be the main thread");
});

test("luaL_ref returns a positive ref", L => {
	lua_pushstring(L, "stored");
	const ref = luaL_ref(L, LUA_REGISTRYINDEX);
	assert.ok(ref > 0, `expected a positive ref, got ${ref}`);
});

test("luaL_ref on nil returns LUA_REFNIL", L => {
	lua_pushnil(L);
	expectEqual(luaL_ref(L, LUA_REGISTRYINDEX), LUA_REFNIL);
});

test("luaL_unref accepts LUA_REFNIL and LUA_NOREF", L => {
	luaL_unref(L, LUA_REGISTRYINDEX, LUA_REFNIL);
	luaL_unref(L, LUA_REGISTRYINDEX, LUA_NOREF);
	expectEqual(lua_gettop(L), 0);
});

test("registry supports custom keys via lua_rawset / lua_rawget", L => {
	lua_pushstring(L, "pluto-test-key");
	lua_pushinteger(L, 99n);
	lua_rawset(L, LUA_REGISTRYINDEX);
	expectEqual(lua_gettop(L), 0, "rawset should pop key and value");

	lua_pushstring(L, "pluto-test-key");
	expectEqual(lua_rawget(L, LUA_REGISTRYINDEX), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 99n);
});

test("luaL_checkinteger reads an integer argument", L => {
	lua_pushinteger(L, 7n);
	expectEqual(luaL_checkinteger(L, 1), 7n);
});

test("luaL_optinteger returns the default for a missing argument", L => {
	expectEqual(luaL_optinteger(L, 1, 9n), 9n);
});

test("luaL_checknumber and luaL_optnumber", L => {
	lua_pushnumber(L, 1.5);
	expectEqual(luaL_checknumber(L, 1), 1.5);
	expectEqual(luaL_optnumber(L, 2, 2.5), 2.5);
});

test("luaL_checkstring and luaL_optstring", L => {
	lua_pushstring(L, "abc");
	expectEqual(luaL_checkstring(L, 1), "abc");
	expectEqual(luaL_optstring(L, 2, "dflt"), "dflt");
});

test("luaL_gsub replaces every occurrence and pushes the result", L => {
	expectEqual(luaL_gsub(L, "a-b-c", "-", "+"), "a+b+c");
	expectEqual(lua_gettop(L), 1);
});

test("luaL_tolstring converts a number and pushes the result", L => {
	lua_pushinteger(L, 42n);
	expectEqual(luaL_tolstring(L, 1, 0), "42");
	expectEqual(lua_gettop(L), 2);
});

test("luaL_len returns the length of a string", L => {
	lua_pushstring(L, "hello");
	expectEqual(luaL_len(L, 1), 5n);
});

test("luaL_getmetafield returns LUA_TNIL without a metatable", L => {
	lua_pushinteger(L, 1n);
	expectEqual(luaL_getmetafield(L, 1, "__index"), LUA_TNIL);
	expectEqual(lua_gettop(L), 1, "nothing should be pushed");
});

test("luaL_newmetatable registers a metatable only once", L => {
	expectEqual(luaL_newmetatable(L, "pluto-test-mt"), 1);
	lua_pop(L, 1);
	expectEqual(luaL_newmetatable(L, "pluto-test-mt"), 0);
	expectEqual(lua_type(L, -1), LUA_TTABLE, "existing metatable should be left on top");
});

test("luaL_getsubtable creates a table once and then reuses it", L => {
	expectEqual(luaL_getsubtable(L, LUA_REGISTRYINDEX, "pluto-test-subtable"), 0);
	expectEqual(lua_type(L, -1), LUA_TTABLE);
	lua_pop(L, 1);
	expectEqual(luaL_getsubtable(L, LUA_REGISTRYINDEX, "pluto-test-subtable"), 1);
	expectEqual(lua_type(L, -1), LUA_TTABLE);
});

test("luaL_loadfilex reports LUA_ERRFILE for a missing file", L => {
	expectEqual(luaL_loadfilex(L, "pluto-test-does-not-exist.lua", 0), LUA_ERRFILE);
});

test("luaL_dostring runs a chunk", L => {
	luaL_openlibs(L);
	expectEqual(luaL_dostring(L, "pluto_test_global = 3"), LUA_OK);
	expectEqual(lua_getglobal(L, "pluto_test_global"), LUA_TNUMBER);
});

test("malloc returns memory that lua_resume can write nres into", L => {
	const co = lua_newthread(L);
	expectEqual(luaL_loadstring(co, "return 1, 2, 3"), LUA_OK);

	const nres = malloc(4);
	assert.ok(nres !== 0, "malloc should return a non-null pointer");
	expectEqual(lua_resume(co, 0, 0, nres), LUA_OK);
	expectEqual(mod.getValue(nres, "i32"), 3, "nres should report the three results");

	free(nres);
});

if (failures > 0) {
	console.error(`${failures} test(s) failed`);
	process.exit(1);
}

console.log("OK");
