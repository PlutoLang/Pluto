import { strict as assert } from "node:assert";

// @ts-expect-error no declaration file
import libpluto from "../libpluto.js";
import wrapper from "../bindings/libpluto-wrapper.mjs";

const mod = await (await libpluto)();
const { luaL_newstate, luaL_openselectedlibs, luaL_openlibs, luaL_loadstring, lua_callk, lua_tolstring } = wrapper(mod);

const L = luaL_newstate();
luaL_openlibs(L);
luaL_loadstring(L, `return require"pluto:base64".encode("Hello")`);
lua_callk(L, 0, 1, 0, 0);
assert(lua_tolstring(L, -1, 0) == "SGVsbG8=");

console.log("OK");
