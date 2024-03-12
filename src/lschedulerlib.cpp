#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_scheduler (lua_State *L) {
	const auto code = R"EOC(pluto_use "0.6.0"

local scheduler = {}

local coros = {}
local function resume(coro)
    if select("#", coroutine.xresume(coro)) ~= 0 then
        warn("Coroutine yielded values to scheduler lib. Discarding them.")
    end
end
local function add(f)
    local coro = coroutine.create(f)
    table.insert(coros, coro)
    resume(coro)
    return coro
end
scheduler.add = add
function scheduler.addloop(f)
    return add(function()
        while f() ~= false do
            coroutine.yield()
        end
    end)
end
scheduler.yieldfunc = || -> do os.sleep(1) end
function scheduler.run()
    local all_dead
    repeat
        all_dead = true
        for i, coro in coros do
            if coroutine.status(coro) == "suspended" then
                resume(coro)
                all_dead = false
            elseif coroutine.status(coro) == "dead" then
                coros[i] = nil
            end
        end
        scheduler.yieldfunc()
    until all_dead
end
return scheduler)EOC";
	luaL_loadbuffer(L, code, strlen(code), "pluto:scheduler");
	lua_call(L, 0, 1);
	return 1;
}

const Pluto::PreloadedLibrary Pluto::preloaded_scheduler{ "scheduler", funcs, &luaopen_scheduler };
