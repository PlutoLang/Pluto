import { strict as assert } from "node:assert";

// @ts-expect-error no declaration file
import libpluto from "../libpluto.js";
import wrapper from "../bindings/libpluto-wrapper.mjs";

const mod = await (await libpluto)();
const {
	LUA_TNIL, LUA_TBOOLEAN, LUA_TNUMBER, LUA_TSTRING,
	luaL_newstate, luaL_openlibs, luaL_loadstring, lua_callk, lua_tolstring, lua_close,
	lua_pushnil, lua_pushstring, lua_pushlstring, lua_pushinteger, lua_pushnumber, lua_pushboolean,
	lua_pushvalue, lua_type, lua_istrue, lua_isinteger, lua_tointegerx, lua_tonumberx,
	lua_gettop, lua_settop, lua_pop, lua_setglobal, lua_getglobal,
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

if (failures > 0) {
	console.error(`${failures} test(s) failed`);
	process.exit(1);
}

console.log("OK");
