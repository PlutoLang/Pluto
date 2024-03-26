#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_scheduler (lua_State *L) {
	const auto code = R"EOC(pluto_use "0.6.0"

local function resume(coro)
    if select("#", coroutine.xresume(coro)) ~= 0 then
        warn("Coroutine yielded values to scheduler. Discarding them.")
    end
end

return class
    __name = "pluto:scheduler"

    yieldfunc = || -> do os.sleep(1) end

    function __construct()
        self.coros = {}
    end

    function add(t)
        if type(t) ~= "thread" then
            t = coroutine.create(t)
        end
        table.insert(self.coros, t)
        resume(t)
        return t
    end

    function addloop(f)
        return self:add(function()
            while f() ~= false do
                coroutine.yield()
            end
        end)
    end

    function run()
        local all_dead
        repeat
            all_dead = true
            for i, coro in self.coros do
                if coroutine.status(coro) == "suspended" then
                    resume(coro)
                    all_dead = false
                elseif coroutine.status(coro) == "dead" then
                    self.coros[i] = nil
                end
            end
            self.yieldfunc()
        until all_dead
    end
end)EOC";
	luaL_loadbuffer(L, code, strlen(code), "pluto:scheduler");
	lua_call(L, 0, 1);
	return 1;
}

const Pluto::PreloadedLibrary Pluto::preloaded_scheduler{ "scheduler", funcs, &luaopen_scheduler };
