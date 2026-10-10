const assert = (b) => {
	if (!b) {
		console.log("ASSERTION FAILED");
		process.exit(1);
	}
};

import libpluto from "../libpluto.js";
const mod = await (await libpluto)();
const luaL_newstate = mod.cwrap("luaL_newstate", "int", []);
const luaL_openselectedlibs = mod.cwrap("luaL_openselectedlibs", "int", ["int", "int", "int"]);
const luaL_openlibs = (L) => luaL_openselectedlibs(L, 1023, 0xffffffff);
const luaL_loadstring = mod.cwrap("luaL_loadstring", "int", ["int", "string"]);
const lua_callk = mod.cwrap("lua_callk", "void", ["int", "int", "int", "int", "int"]);
const lua_tolstring = mod.cwrap("lua_tolstring", "string", ["int", "int", "int"]);

const L = luaL_newstate();
luaL_openlibs(L);
luaL_loadstring(L, `return require"pluto:base64".encode("Hello")`);
lua_callk(L, 0, 1, 0, 0);
assert(lua_tolstring(L, -1, 0) == "SGVsbG8=");

console.log("OK");
