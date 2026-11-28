import assert from "node:assert";
import { Buffer } from "node:buffer";
import pluto from "../pluto.js";

// Hook stdout. Emscripten passes each output byte as a (possibly negative) number.
const config = {};
config.noInitialRun = true;
const outputBytes = [];
config.preRun = function(mod)
{
	const out = function(c)
	{
		outputBytes.push(c & 0xff);
	};
	mod.FS.init(undefined, out, out);
};

// Init wasm
const mod = await (await pluto)(config);

// Write index.pluto into virtual filesystem
{
	const data = Buffer.from("print('Hello, world!')", "utf8");
	const stream = mod.FS.open("index.pluto", "w+");
	mod.FS.write(stream, data, 0, data.length, 0);
	mod.FS.close(stream);
}

// Final push, run "pluto index.pluto"
const prog = {
	mod: mod,
	malloc: mod.cwrap("malloc", "int", ["int"]),
	free: mod.cwrap("free", "void", ["int"]),
	strcpy: mod.cwrap("strcpy", "void", ["int", "string"]),
	main: mod.cwrap("main", "int", ["int", "int"]),
};

function allocateString(prog, str)
{
	const ptr = prog.malloc(Buffer.byteLength(str, "utf8") + 1);
	prog.strcpy(ptr, str);
	return ptr;
}

const PTRSIZE = 4;
function allocateStringArray(prog, arr)
{
	const ptr = prog.malloc(PTRSIZE * arr.length);
	for (let i = 0; i != arr.length; ++i)
	{
		prog.mod.setValue(ptr + i * PTRSIZE, allocateString(prog, arr[i]), "i" + (PTRSIZE * 8));
	}
	return ptr;
};

let argv = [ "pluto", "index.pluto" ];
let argv_ptr = allocateStringArray(prog, argv);
let status = prog.main(argv.length, argv_ptr);
assert.strictEqual(status, 0);
assert.strictEqual(Buffer.from(outputBytes).toString("utf8"), "Hello, world!\n");
