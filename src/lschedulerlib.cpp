#include <cstring>

#define LUA_LIB
#include "lualib.h"

static const luaL_Reg funcs[] = {
  {nullptr, nullptr}
};

LUAMOD_API int luaopen_scheduler (lua_State *L) {
#ifdef PLUTO_DONT_LOAD_ANY_STANDARD_LIBRARY_CODE_WRITTEN_IN_PLUTO
    return 0;
#else
    const auto code = R"EOC(pluto_use "0.6.0"

return class
    __name = "pluto:scheduler"

    yieldfunc = || -> do os.sleep(1) end

    function __construct()
        self.coros = {}
    end

    function internalresume(coro)
        if not self.errorfunc then
            if select("#", coroutine.xresume(coro)) ~= 0 then
                warn("Coroutine yielded values to scheduler. Discarding them.")
            end
        else
            local ok, val = coroutine.resume(coro)
            if ok then
                if val ~= nil then
                    warn("Coroutine yielded values to scheduler. Discarding them.")
                end
            else
                self.errorfunc(val)
            end
        end
    end

    function add(t)
        if type(t) ~= "thread" then
            if self.errorfunc then
                local f = t
                t = coroutine.create(function()
                    local r = table.pack(xpcall(f, self.errorfunc))
                    if r[1] and r.n ~= 1 then
                        warn("Coroutine yielded values to scheduler. Discarding them.")
                    end
                end)
            else
                t = coroutine.create(t)
            end
        end
        table.insert(self.coros, t)
        self:internalresume(t)
        return t
    end

    function addloop(f)
        return self:add(function()
            while f() ~= false do
                coroutine.yield()
            end
        end)
    end

    function contains(needle)
        for self.coros as t do
            if t == needle then
                return true
            end
        end
        return false
    end

    function remove(needle)
        for i, t in self.coros do
            if t == needle then
                table.remove(self.coros, i)
                break
            end
        end
    end

    function run()
        local all_dead
        repeat
            all_dead = true
            for i, coro in self.coros do
                if coroutine.status(coro) == "suspended" then
                    self:internalresume(coro)
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
#endif
}

const Pluto::PreloadedLibrary Pluto::preloaded_scheduler{ "scheduler", funcs, &luaopen_scheduler };
