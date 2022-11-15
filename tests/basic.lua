do
    local a = 1
    local a = 2
end
print "Welcome to the test suit. There should have been exactly 1 parser warning."

print "Testing compound assignment."
do
    local a, b = 1, 2
    a += 1
    b += 1
    assert(a == 2)
    assert(b == 3)

    a, b = 1, 2
    a -= 1
    b -= 1
    assert(a == 0)
    assert(b == 1)

    a, b = 1, 2
    a *= 2
    b *= 2
    assert(a == 2)
    assert(b == 4)

    a, b = 1, 2
    a %= 2
    b %= 2
    assert(a == 1)
    assert(b == 0)

    a, b = 1, 2
    a ^= 2
    b ^= 2
    assert(a == 1.0)
    assert(b == 4.0)

    a, b = 1, 2
    a **= 2
    b **= 2
    assert(a == 1.0)
    assert(b == 4.0)

    a, b = 1, 2
    a |= 1
    b |= 1
    assert(a == 1)
    assert(b == 3)

    a, b = 1, 2
    a &= 1
    b &= 1
    assert(a == 1)
    assert(b == 0)

    a, b = 1, 2
    a /= 1
    b /= 2
    assert(a == 1.0)
    assert(b == 1.0)

    a, b = 1, 2
    a //= 1
    b //= 2
    assert(a == 1)
    assert(b == 1)

    a, b = 1, 2
    a <<= 1
    b <<= 1
    assert(a == 2)
    assert(b == 4)

    a, b = 1, 2
    a >>= 1
    b >>= 1
    assert(a == 0)
    assert(b == 1)
end

print "Testing string indexing."
do
    local str = "abc"
    assert(str[0] == nil)
    assert(str[1] == "a")
    assert(str[2] == "b")
    assert(str[3] == "c")
    assert(str[4] == nil)
    assert(str[5] == nil)
    assert(str[-1] == "c")
    assert(str[-2] == "b")
    assert(str[-3] == "a")
    assert(str[-4] == nil)
    assert(str[-5] == nil)
end

print "Testing continue statement."
do
    local t = { 1, 2, 3, 4, 5, 6, 7, 8, 9 }
    local sum = 0
    for index, value in ipairs(t) do
        if value == 5 then
            continue
        end
        sum += value
    end
    assert(sum == 40)

    sum = 0
    for i = 1, 10 do
        if i == 5 then
            continue
        end
        sum += i
    end
    assert(sum == 50)

    local lines = {
        "This",
        "Is",
        "Some",
        "Lines",
    }

    for index, line in ipairs(lines) do
        if index == 1 and line == "This" then
            continue
        elseif index == #lines and line == "Lines" then
            continue
        end
        assert(line != "This" and line != "Lines")
    end
end

print "Testing table length cache."
do
    local t = {}
    for i = 1, 1000 do
        t[i] = "abc"
    end
    assert(#t == 1000, #t)

    for i = 1, 100 do
        table.remove(t, i + math.random(1, 5))
    end
    assert(#t == 900, #t)

    local metatest = setmetatable({}, { __len = function () return 5 end })
    assert(#metatest == 5, #metatest)

    t = {}
    for i = 1, 1000 do
        table.insert(t, "Hello")
        table.insert(t, "World")
    end
    assert(#t == 2000)

    t = {}
    for i = 1, 1000 do
        rawset(t, i, "Hello")
    end
    assert(#t == 1000)
end

print "Testing null coalescing operator."
do
    _G.zz = false
    z = _G.zz
    a = z
    b = "hello"
    c = a ?? b
    assert(c == false)
    _G.zz = nil
    z = _G.zz
    a = z
    b = "hello"
    c = a ?? b
    assert(c == "hello")
    local zz = false
    local z = zz
    local a = z
    local b = "hello"
    local c = a ?? b
    assert(c == false)
    zz = nil
    z = zz
    a = z
    b = "hello"
    c = a ?? b
    assert(c == "hello")
    a = false
    b = "hello"
    a ??= b
    assert(a == false)
    a = nil
    a ??= b
    assert(a == "hello")
end

print "Testing safe navigation."
do
    local a = A?.B?.C?.D
    assert(a == nil)
    a = A?["B"]?["C"]?["D"]
    assert(a == nil)
    a = A?["B"]?["C"]?["D"]?[-5]?[0]
    local T = {}
    T.K = {}
    T.K.Z = {}
    assert(T?.K?.Z == T.K.Z)
end

print "Testing shorthand ternary."
do
    local a = 3
    assert((true ? "yes" : "no") == "yes")
    assert((false ? "yes" : "no") == "no")
    assert((a ? "yes" : "no") == "yes")
    assert((a == 3 ? "yes" : "no") == "yes")
    assert((a == 4 ? "yes" : "no") == "no")
    assert((3 == a ? "yes" : "no") == "yes")
    assert((4 == a ? "yes" : "no") == "no")
end

print "Testing new 'in' expressions."
if ("hel" in "hello") != true then error() end
if ("abc" in "hello") != false then error() end

print "Testing break N syntax."
do
    local sum = 0
    for i = 1, 10 do
        for ii = 1, 10 do
            sum = sum + ii + i
            break 1
        end
        sum = sum + i
    end
    assert(sum == 120)
end

do
    local sum = 0
    for i = 1, 10 do
        for ii = 1, 10 do
            sum = sum + ii + i
            break 2
        end
        sum = sum + i
    end
    assert(sum == 2)
end

do
    local sum = 0
    for i = 1, 10 do
        for ii = 1, 10 do
            sum = sum + ii + i
            for iii = 1, 10 do
                sum = sum + iii + ii + i
                break 3
            end
        end
        sum = sum + i
    end
    assert(sum == 5)
end

do
    while true do
        if true then
            break
        end
    end
end

print "Testing switch statement."
do
    local value = 5
    pluto_switch (value) do
        case 5:
        break
        default:
        error()
    end
    value = 3
    pluto_switch value do
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        break
        default:
        error()
    end
    do
        local casecond <const> = 3
        pluto_switch value do
            case casecond:
            break
            default:
            error()
        end
    end
    do
        local casecond = 3
        pluto_switch value do
            case casecond:
            break
            default:
            error()
        end
    end
    do
        local casecond <const> = 3
        pluto_switch value do
            default:
            error()
            case casecond:
        end
    end
    do
        local casecond = 3
        pluto_switch value do
            default:
            error()
            case casecond:
        end
    end
    value = +3
    pluto_switch value do
        case +1:
        case +2:
        case +3:
        case +4:
        case +5:
        break
        default:
        error()
    end
    value = "foo"
    pluto_switch (value) do
        case "foo":
        break
        default:
        error()
    end
    pluto_switch (value) do
        case "abc":
        case "124":
        case nil:
        case false:
        case true:
        case "23420948239":
        case "foo":
        case 1238123:
        case -2409384029842:
        case "awweee":
        break
        default:
        error()
    end
    value = nil
    pluto_switch (value) do
        case -1:
        case nil:
        case -2:
        break
        default:
        error()
    end
    value = -24389
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case 231923:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        break
        default: 
        error()
    end
    value = -1
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case 231923:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        error()
    end
    value = -3.14
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case -3.14:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        break
    end
    value = -3.3
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case -3.15:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        error()
    end
    t = 0
    value = -3.15
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case -3.15:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        t = true
    end
    assert(t == true)
    t = 0
    value = -3.15
    pluto_switch (value) do
        case "aawdkawmlwadmlaw":
        case "q49324932":
        case nil:
        case "130-91230921":
        case false:
        case -3.15:
        case true:
        case -234234:
        case -24389:
        case 23429:
        case "bar":
        case "foobar":
        case "barfoo":
        t = true
        break
        default:
        t = false
    end
    assert(t == true)
    t = 0
    value = 3
    pluto_switch value do
        case 1:
        default:
        error()
        case 3:
        t = true
    end
    assert(t == true)
end

print "Testing table freezing."
do
    local t = table.freeze({ 1, 2, 3, "hello", "world" })
    local status, _ = pcall(function () t.key = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () t["key"] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () t[1] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () t[66] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () t[function () end] = "abc" end)
    assert(status == false, "expected error")

    t = { 1, 2, 3, "hello", "world" }
    status, _ = pcall(function () t.key = "abc" end)
    assert(status == true, "unexpected error")
    status, _ = pcall(function () t["key"] = "abc" end)
    assert(status == true, "unexpected error")
    status, _ = pcall(function () t[1] = "abc" end)
    assert(status == true, "unexpected error")
    status, _ = pcall(function () t[66] = "abc" end)
    assert(status == true, "unexpected error")
    status, _ = pcall(function () t[function () end] = "abc" end)
    assert(status == true, "unexpected error")

    --[[
    table.freeze(_G)
    status, _ = pcall(function () _G.string = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _G["string"] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _G[1] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _G[66] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _G[function () end] = "abc" end)
    assert(status == false, "expected error")

    table.freeze(_ENV)
    status, _ = pcall(function () _ENV.string = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _ENV["string"] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _ENV[1] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _ENV[66] = "abc" end)
    assert(status == false, "expected error")
    status, _ = pcall(function () _ENV[function () end] = "abc" end)
    assert(status == false, "expected error")
    --]] --> Doing this will break tests done with dofile(), since the environment is reused.
end

print "Testing standard library additions."
do
    local crypto = require("crypto")
    assert(crypto.fnv1("hello world") == 0x7DCF62CDB1910E6F)
    assert(crypto.fnv1a("hello world") == 8618312879776256743)
    assert(crypto.joaat("hello world") == 1045060183)
    assert(crypto.joaat("hello world") == tonumber(crypto.hexdigest(crypto.joaat("hello world"))))
end
do
    if package.preload["base64"] ~= nil then -- Soup is linked.
        do
            local base64 = require("base64")
            assert(base64.encode("Hello") == "SGVsbG8")
            assert(base64.decode("SGVsbG8") == "Hello")
            assert(base64.urlEncode("Hello") == "SGVsbG8")
            assert(base64.urlDecode("SGVsbG8") == "Hello")
            assert(base64.decode(base64.encode("Hello", true)) == "Hello")
        end
    end
end
do
    local t = {}
    table.insert(t, 0)
    table.insert(t, "Hello")
    table.insert(t, true)
    assert(table.contains(t, "Hello") == 2)
    assert(table.contains(t, "World") == nil)
    assert(table.contains(t, true) == 3)
    assert(table.contains(t, false) == nil)
    assert(table.contains(t, 0) == 1)
    assert(table.contains(t, 1) == nil)
    assert(string.isascii("hello world") == true)
    assert(string.isascii("hello.world") == true)
    assert(string.isascii("hello1world") == true)
    assert(string.isascii("hello📙world") == false)
    assert(string.islower("hello world") == false)
    assert(string.islower("helloworld") == true)
    assert(string.islower("hello1world") == false)
    assert(string.isalpha("hello world") == false)
    assert(string.isalpha("helloworld") == true)
    assert(string.isalpha("hello1world") == false)
    assert(string.isalpha("hello?world") == false)
    assert(string.isupper("HELLOWORLD") == true)
    assert(string.isupper("HELLO WORLD") == false)
    assert(string.isupper("HELLO?WORLD") == false)
    assert(string.isalnum("abc123") == true)
    assert(string.isalnum("abc 123") == false)
    assert(string.isalnum("abc?123") == false)
    assert(string.iswhitespace("   \t   \f \n \r\n") == true)
    assert(string.iswhitespace("\t\f   \r\n \r \n \t z") == false)
    assert(string.contains("hello world", "world") == true)
    assert(string.contains("hello world", "z") == false)
    assert(string.endswith("hello world", "rld") == true)
    assert(string.endswith("hello world", "trc") == false)
    assert(string.startswith("hello world", "hello") == true)
    assert(string.startswith("hello world", "truck") == false)
    assert(string.strip("???hello world???", "?") == "hello world")
    assert(string.strip("123hello world123", "123") == "hello world")
    assert(string.lstrip("???hello world???", "?") == "hello world???")
    assert(string.lstrip("12hello world12", "12") == "hello world12")
    assert(string.rstrip("???hello world???", "?") == "???hello world")
    t = string.split("hello world abc", " ")
    assert(t[1] == "hello")
    assert(t[2] == "world")
    assert(t[3] == "abc")
    local before, after = string.partition("hello.wor.ld", ".")
    assert(before == "hello")
    assert(after == "wor.ld")
    before, after = string.partition("hello.wor.ld", ".", true)
    assert(before == "hello.wor")
    assert(after == "ld")
    assert(string.casefold("HELLO WORLD", "hello world") == true)
    assert(string.casefold("HELLO WORLD", "hello worlz") == false)
    assert(string.lfind("hello world", "wor") == (string.find("hello world", "wor", 1, true)))
    assert(string.rfind("world hello world", "world") == 13)
    assert(string.rfind("hello x", "world") == nil)
    assert(string.find_first_of("hello world", "?[[w") == 7)
    assert(string.find_first_of("hello world", "?[[z") == nil)
    assert(string.find_first_not_of("hello world", "hello") == 6)
    assert(string.find_first_not_of("hello world", "hello world") == nil)
    assert(string.find_last_of("orld hello world cccc", "orld") == 16)
    assert(string.upper("hello", 1) == "Hello")
    assert(string.upper("hello", 2) == "hEllo")
    assert(string.upper("hello", -1) == "hellO")
    assert(string.upper("hello", -2) == "helLo")
    assert(string.upper("hello", -6) == "hello")
    assert(string.upper("hello", 6) == "hello")
    assert(string.upper("hello") == "HELLO")
    assert(string.lower("HELLO") == "hello")
    assert(string.lower("HELLO", 1) == "hELLO")
    assert(string.lower("HELLO", 2) == "HeLLO")
    assert(string.lower("HELLO", -1) == "HELLo")
    assert(string.lower("HELLO", -2) == "HELlO")
    assert(string.lower("HELLO", -14) == "HELLO")
end

print "Testing walrus operator."
do
    if a := 3 then
    else
        error()
    end
    assert(a == 3)

    if b := nil then
        error()
    end

    -- Complex Context: Walrus in function body of a lambda function that is passed an argument
    local function executeFunc(f)
        f()
    end
    executeFunc(function()
        if c := 3 then
            assert(c == 3)
        else
            error()
        end
    end)

    --[[local function walrus_test_helper(v1, v2)
        assert(v1 == "hi")
        assert(v2 == nil)
    end
    walrus_test_helper(c := "hi")]]
end

print "Testing for-as loop."
do
    local t = { "a", "b", "c" }
    local k = 1
    for t as v do
        assert(t[k] == v)
        k = k + 1
    end
end

print "Testing format strings."
do
    f_string_global = "foo"
    local f_string_local = "bar"
    assert($"a{f_string_global}b{f_string_local}c" == "afoobbarc")
    assert($"{f_string_global}{f_string_local}" == "foobar")
end

print "Testing ++ operator."
do
    local a = 1
    assert(++a == 2)
    assert(a == 2)
end

print "Testing non-ascii variable names."
do
    local 😉 = "Hello"
    assert(😉 == "Hello")
end

print "Testing binary numerals."
do
    assert(0b11 == 3)
end

print "Testing compatibility."
do
    local a = "Hi"
    local t = {a}
    assert(t[1] == "Hi")
    t = {a, nil}
    assert(t[1] == "Hi")
    t = {["func"]=function(p1,p2)end}
    assert(t["func"] ~= nil)

    local myconst <const> = "consty"
    assert(myconst == "consty")

    if true then
        goto if_then_goto_test
        ::if_then_goto_test::
    end

    local function compat_default_name(default)
        assert(default == "yes")
    end
    compat_default_name("yes")
end
do
    local t = { "a", "b", "c" }
    for k, v in t do
        assert(t[k] == v)
    end
end

print "Finished."
