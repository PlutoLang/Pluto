import { strict as assert } from "node:assert";

// @ts-expect-error no declaration file
import libpluto from "../libpluto.js";
import wrapper from "./libpluto-wrapper.mjs";

const mod = await (await libpluto)();
const {
	LUA_TNIL, LUA_TBOOLEAN, LUA_TNUMBER, LUA_TSTRING, LUA_TTABLE, LUA_TFUNCTION, LUA_ERRRUN,
	LUA_OPADD, LUA_OPLT,
	luaL_newstate, luaL_openlibs, luaL_loadstring, lua_callk, lua_tolstring, lua_close,
	lua_pushnil, lua_pushstring, lua_pushlstring, lua_pushinteger, lua_pushnumber, lua_pushboolean,
	lua_pushvalue, lua_type, lua_istrue, lua_isinteger, lua_tointegerx, lua_tonumberx,
	lua_gettop, lua_settop, lua_pop, lua_setglobal, lua_getglobal,
	lua_typename, lua_absindex, lua_rotate, lua_copy, lua_arith, lua_compare,
	lua_createtable, lua_getfield, lua_setfield, lua_geti, lua_seti, lua_next,
	lua_concat, lua_len, lua_stringtonumber, lua_pcall, lua_pushglobaltable,
	lua_isnil, lua_isnoneornil,
	lua_rawgeti, lua_rawseti, lua_rawget, lua_rawset, lua_rawequal, lua_pushthread,
	luaL_ref, luaL_unref,
	LUA_TTHREAD, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS, LUA_RIDX_MAINTHREAD,
	LUA_REFNIL, LUA_NOREF,
} = wrapper(mod);

// ---- Original smoke test ----
{
	const L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadstring(L, `return require"pluto:base64".encode("Hello")`);
	lua_callk(L, 0, 1, 0, 0);
	assert(lua_tolstring(L, -1, 0) == "SGVsbG8=");
	lua_close(L);
}

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

test("lua_pushnumber handles negatives and zero", L => {
	lua_pushnumber(L, -0.25);
	expectEqual(lua_tonumberx(L, -1, 0), -0.25);
	lua_pushnumber(L, 0);
	expectEqual(lua_tonumberx(L, -1, 0), 0);
});

test("lua_pushnumber handles large, infinite and NaN values", L => {
	lua_pushnumber(L, 1e308);
	expectEqual(lua_tonumberx(L, -1, 0), 1e308);
	lua_pushnumber(L, Infinity);
	expectEqual(lua_tonumberx(L, -1, 0), Infinity);
	lua_pushnumber(L, -Infinity);
	expectEqual(lua_tonumberx(L, -1, 0), -Infinity);
	lua_pushnumber(L, NaN);
	assert.ok(Number.isNaN(lua_tonumberx(L, -1, 0)));
});

// lua_pushinteger
test("lua_pushinteger pushes an integer", L => {
	lua_pushinteger(L, 42n);
	expectEqual(lua_type(L, -1), LUA_TNUMBER);
	expectEqual(lua_isinteger(L, -1), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 42n);
	expectEqual(lua_tonumberx(L, -1, 0), 42);
});

test("lua_pushinteger handles negative integers", L => {
	lua_pushinteger(L, -7n);
	expectEqual(lua_isinteger(L, -1), 1);
	expectEqual(lua_tointegerx(L, -1, 0), -7n);
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

test("lua_pushlstring pushes the full buffer when len matches", L => {
	const bytes = new TextEncoder().encode("world");
	lua_pushlstring(L, bytes, bytes.length);
	expectEqual(lua_tolstring(L, -1, 0), "world");
});

// lua_tonumberx / lua_tointegerx on strings
test("lua_tonumberx converts a numeric string", L => {
	lua_pushstring(L, "3.5");
	expectEqual(lua_tonumberx(L, -1, 0), 3.5);
});

test("lua_tointegerx truncates an integral float", L => {
	lua_pushnumber(L, 3.0);
	expectEqual(lua_tointegerx(L, -1, 0), 3n);
});

// lua_pushvalue
test("lua_pushvalue duplicates the value at the given index", L => {
	lua_pushnumber(L, 7.5);
	lua_pushvalue(L, -1);
	expectEqual(lua_gettop(L), 2);
	expectEqual(lua_tonumberx(L, -1, 0), 7.5);
	expectEqual(lua_tonumberx(L, -2, 0), 7.5);
});

// lua_gettop / lua_settop / lua_pop
test("lua_gettop is 0 on a fresh state", L => {
	expectEqual(lua_gettop(L), 0);
});

test("lua_gettop counts pushed values", L => {
	lua_pushinteger(L, 1n);
	lua_pushnil(L);
	lua_pushboolean(L, 1);
	expectEqual(lua_gettop(L), 3);
});

test("lua_settop truncates the stack", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	lua_pushinteger(L, 3n);
	lua_settop(L, 1);
	expectEqual(lua_gettop(L), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 1n);
});

test("lua_settop(L, 0) clears the stack", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	lua_settop(L, 0);
	expectEqual(lua_gettop(L), 0);
});

test("lua_pop removes the top n values", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	lua_pushinteger(L, 3n);
	lua_pop(L, 2);
	expectEqual(lua_gettop(L), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 1n);
});

// lua_setglobal / lua_getglobal
test("lua_setglobal / lua_getglobal round-trip a number", L => {
	lua_pushnumber(L, 1.25);
	lua_setglobal(L, "answer");
	expectEqual(lua_gettop(L), 0, "setglobal should pop the value");

	lua_getglobal(L, "answer");
	expectEqual(lua_type(L, -1), LUA_TNUMBER);
	expectEqual(lua_tonumberx(L, -1, 0), 1.25);
});

// lua_typename
test("lua_typename names basic types", L => {
	expectEqual(lua_typename(L, LUA_TTABLE), "table");
	expectEqual(lua_typename(L, LUA_TFUNCTION), "function");
	expectEqual(lua_typename(L, LUA_TNIL), "nil");
});

// lua_absindex
test("lua_absindex converts a negative index to absolute", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	expectEqual(lua_absindex(L, -1), 2);
	expectEqual(lua_absindex(L, 1), 1);
});

// lua_rotate
test("lua_rotate moves the top value down to the given index", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	lua_pushinteger(L, 3n);
	lua_rotate(L, 1, 1); // stack becomes 3, 1, 2
	expectEqual(lua_tointegerx(L, 1, 0), 3n);
	expectEqual(lua_tointegerx(L, -1, 0), 2n);
});

// lua_copy
test("lua_copy overwrites one slot with another", L => {
	lua_pushinteger(L, 1n);
	lua_pushinteger(L, 2n);
	lua_copy(L, 1, 2);
	expectEqual(lua_gettop(L), 2);
	expectEqual(lua_tointegerx(L, -1, 0), 1n);
});

// lua_arith / lua_compare
test("lua_arith adds the two top values", L => {
	lua_pushinteger(L, 2n);
	lua_pushinteger(L, 3n);
	lua_arith(L, LUA_OPADD);
	expectEqual(lua_gettop(L), 1);
	expectEqual(lua_tointegerx(L, -1, 0), 5n);
});

test("lua_compare reports less-than", L => {
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

// lua_geti / lua_seti
test("lua_seti / lua_geti round-trip an array element", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 7n);
	lua_seti(L, -2, 1n);
	expectEqual(lua_geti(L, -1, 1n), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 7n);
});

// lua_next
test("lua_next iterates every key/value pair of a table", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 1n);
	lua_setfield(L, -2, "x");
	lua_pushinteger(L, 2n);
	lua_setfield(L, -2, "y");
	lua_pushinteger(L, 3n);
	lua_setfield(L, -2, "z");

	const keys: string[] = [];
	let sum = 0n;
	lua_pushnil(L); // initial key
	while (lua_next(L, -2) !== 0) {
		// stack: table, key, value
		keys.push(lua_tolstring(L, -2, 0));
		sum += lua_tointegerx(L, -1, 0);
		lua_pop(L, 1); // drop value, keep key for the next call
	}

	expectEqual(keys.sort().join(","), "x,y,z");
	expectEqual(sum, 6n);
	expectEqual(lua_gettop(L), 1, "lua_next should leave only the table when finished");
});

test("lua_next returns 0 and pushes nothing for an empty table", L => {
	lua_createtable(L, 0, 0);
	lua_pushnil(L);
	expectEqual(lua_next(L, -2), 0);
	expectEqual(lua_gettop(L), 1, "only the table should remain");
});

// lua_concat / lua_len
test("lua_concat joins the top n strings", L => {
	lua_pushstring(L, "a");
	lua_pushstring(L, "b");
	lua_concat(L, 2);
	expectEqual(lua_gettop(L), 1);
	expectEqual(lua_tolstring(L, -1, 0), "ab");
});

test("lua_len pushes the length of a string", L => {
	lua_pushstring(L, "abc");
	lua_len(L, -1);
	expectEqual(lua_tointegerx(L, -1, 0), 3n);
});

// lua_stringtonumber
test("lua_stringtonumber pushes the parsed number and returns size+1", L => {
	expectEqual(lua_stringtonumber(L, "42"), 3);
	expectEqual(lua_tointegerx(L, -1, 0), 42n);
});

test("lua_stringtonumber returns 0 and pushes nothing on bad input", L => {
	expectEqual(lua_stringtonumber(L, "nope"), 0);
	expectEqual(lua_gettop(L), 0);
});

// lua_pcall
test("lua_pcall returns LUA_ERRRUN for a raised error", L => {
	luaL_openlibs(L);
	luaL_loadstring(L, "error('boom')");
	expectEqual(lua_pcall(L, 0, 0, 0), LUA_ERRRUN);
});

test("lua_pcall returns LUA_OK on success", L => {
	luaL_loadstring(L, "return 1 + 1");
	expectEqual(lua_pcall(L, 0, 1, 0), 0);
	expectEqual(lua_tointegerx(L, -1, 0), 2n);
});

// lua_pushglobaltable
test("lua_pushglobaltable exposes the globals table", L => {
	lua_pushglobaltable(L);
	expectEqual(lua_type(L, -1), LUA_TTABLE);
	lua_pushinteger(L, 9n);
	lua_setfield(L, -2, "from_c");
	lua_pop(L, 1);
	expectEqual(lua_getglobal(L, "from_c"), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 9n);
});

// Type-check macros
test("lua_isnil and lua_isnoneornil work on nil and none", L => {
	lua_pushnil(L);
	expectEqual(lua_isnil(L, -1), 1);
	expectEqual(lua_isnoneornil(L, 5), 1);
	expectEqual(lua_isnoneornil(L, -1), 1);
	lua_pushinteger(L, 1n);
	expectEqual(lua_isnil(L, -1), 0);
});

// ---- Registry ----
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

test("luaL_ref stores a value and lua_rawgeti retrieves it", L => {
	lua_pushstring(L, "stored");
	const ref = luaL_ref(L, LUA_REGISTRYINDEX);
	assert.ok(ref > 0, `expected a positive ref, got ${ref}`);
	expectEqual(lua_gettop(L), 0, "luaL_ref should pop the value");
	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(ref)), LUA_TSTRING);
	expectEqual(lua_tolstring(L, -1, 0), "stored");
});

test("luaL_ref keeps tables reachable through their ref", L => {
	lua_createtable(L, 0, 0);
	lua_pushinteger(L, 11n);
	lua_setfield(L, -2, "x");
	const ref = luaL_ref(L, LUA_REGISTRYINDEX);
	expectEqual(lua_gettop(L), 0);
	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(ref)), LUA_TTABLE);
	expectEqual(lua_getfield(L, -1, "x"), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 11n);
});

test("luaL_ref returns distinct refs for distinct values", L => {
	lua_pushinteger(L, 1n);
	const a = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushinteger(L, 2n);
	const b = luaL_ref(L, LUA_REGISTRYINDEX);
	assert.notEqual(a, b);

	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(a)), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 1n);
	expectEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(b)), LUA_TNUMBER);
	expectEqual(lua_tointegerx(L, -1, 0), 2n);
});

test("luaL_ref on nil returns LUA_REFNIL and pops the value", L => {
	lua_pushnil(L);
	expectEqual(luaL_ref(L, LUA_REGISTRYINDEX), LUA_REFNIL);
	expectEqual(lua_gettop(L), 0);
});

test("luaL_unref releases a ref so its value is no longer retrievable", L => {
	lua_pushstring(L, "temp");
	const ref = luaL_ref(L, LUA_REGISTRYINDEX);
	luaL_unref(L, LUA_REGISTRYINDEX, ref);
	// Lua reuses the slot for its free list, so it must no longer hold the string.
	assert.notEqual(lua_rawgeti(L, LUA_REGISTRYINDEX, BigInt(ref)), LUA_TSTRING);
});

test("luaL_unref ignores LUA_REFNIL and LUA_NOREF", L => {
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
