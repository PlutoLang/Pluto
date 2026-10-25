import { strict as assert } from "node:assert";

// @ts-expect-error no declaration file
import libpluto from "../libpluto.js";
import wrapper from "./libpluto-wrapper.mjs";

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
	luaL_ref, luaL_unref,
	LUA_TTHREAD, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS, LUA_RIDX_MAINTHREAD,
	LUA_REFNIL, LUA_NOREF,
} = wrapper(mod);

// ---- Unit tests: each runs on a fresh Lua state ----
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

// lua_pushnumber
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

// lua_pushinteger
test("lua_pushinteger pushes an integer", L => {
	lua_pushinteger(L, 42n);
	expectEqual(lua_type(L, -1), LUA_TNUMBER);
	expectEqual(lua_isinteger(L, -1), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 42n);
	expectEqual(lua_tonumberx(L, -1, 0), 42);
});

// lua_pushnil
test("lua_pushnil pushes nil", L => {
	lua_pushnil(L);
	expectEqual(lua_type(L, -1), LUA_TNIL);
	expectEqual(lua_gettop(L), 1);
});

// lua_pushboolean / lua_istrue
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

// lua_pushstring / lua_pushlstring
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

// lua_tointegerx return type
test("lua_tointegerx returns a bigint", L => {
	lua_pushnumber(L, 3.0);
	expectEqual(lua_tointegerx(L, -1, 0), 3n);
});

// lua_gettop return type
test("lua_gettop counts pushed values", L => {
	lua_pushinteger(L, 1n);
	lua_pushnil(L);
	lua_pushboolean(L, 1);
	expectEqual(lua_gettop(L), 3);
});

// lua_setglobal / lua_getglobal
test("lua_setglobal / lua_getglobal round-trip a number", L => {
	lua_pushnumber(L, 1.25);
	lua_setglobal(L, "answer");
	expectEqual(lua_gettop(L), 0, "setglobal should pop the value");

	expectEqual(lua_getglobal(L, "answer"), LUA_TNUMBER);
	expectEqual(lua_tonumberx(L, -1, 0), 1.25);
});

// lua_typename
test("lua_typename names basic types", L => {
	expectEqual(lua_typename(L, LUA_TTABLE), "table");
	expectEqual(lua_typename(L, LUA_TFUNCTION), "function");
	expectEqual(lua_typename(L, LUA_TNIL), "nil");
});

// lua_arith / lua_compare (constants)
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

// lua_createtable / lua_getfield / lua_setfield
test("lua_setfield / lua_getfield round-trip a table field", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 5n);
	lua_setfield(L, -2, "x");
	expectEqual(lua_gettop(L), 1, "setfield should pop the value");
	expectEqual(lua_getfield(L, -1, "x"), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 5n);
});

// lua_geti / lua_seti (bigint index argument)
test("lua_seti / lua_geti round-trip an array element", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 7n);
	lua_seti(L, -2, 1n);
	expectEqual(lua_geti(L, -1, 1n), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 7n);
});

// lua_next return type
test("lua_next returns 0 and pushes nothing for an empty table", L => {
	lua_createtable(L, 0, 0);
	lua_pushnil(L);
	expectEqual(lua_next(L, -2), 0);
	expectEqual(lua_gettop(L), 1, "only the table should remain");
});

// lua_stringtonumber return type
test("lua_stringtonumber returns size+1 for a parsed number", L => {
	expectEqual(lua_stringtonumber(L, "42"), 3);
});

// lua_pcall / LUA_ERRRUN constant
test("lua_pcall returns LUA_ERRRUN for a raised error", L => {
	luaL_openlibs(L);
	luaL_loadstring(L, "error('boom')");
	expectEqual(lua_pcall(L, 0, 0, 0), LUA_ERRRUN);
});

// lua_isnil / lua_isnoneornil return types
test("lua_isnil and lua_isnoneornil work on nil and none", L => {
	lua_pushnil(L);
	expectEqual(lua_isnil(L, -1), 1);
	expectEqual(lua_isnoneornil(L, 5), 1);
	expectEqual(lua_isnoneornil(L, -1), 1);
	lua_pushinteger(L, 1n);
	expectEqual(lua_isnil(L, -1), 0);
});

// ---- Registry constants and argument/return types ----
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

if (failures > 0) {
	console.error(`${failures} test(s) failed`);
	process.exit(1);
}

console.log("OK");
