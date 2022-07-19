print "Testing compound assignment."
a, b = 1, 2
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

print "Testing string indexing."
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

print "Testing continue statement."
local t = { 1, 2, 3, 4, 5, 6, 7, 8, 9 }
local sum = 0
for index, value in ipairs(t) do
    if value == 5 then
        continue
    end
    sum += value
end
assert(sum == 40)

local sum = 0
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

print "Testing table length cache."
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

print "Testing switch statement."
local value = 5
pluto_switch (value) do
    pluto_case 5:
        break
    pluto_default:
        error()
end
value = 3
pluto_switch value do
    pluto_case 1:
    pluto_case 2:
    pluto_case 3:
    pluto_case 4:
    pluto_case 5:
        break
    pluto_default:
        error()
end
value = "foo"
pluto_switch (value) do
    pluto_case "foo":
        break
    pluto_default:
        error()
end
pluto_switch (value) do
    pluto_case "abc":
    pluto_case "124":
    pluto_case nil:
    pluto_case false:
    pluto_case true:
    pluto_case "23420948239":
    pluto_case "foo":
    pluto_case 1238123:
    pluto_case -2409384029842:
    pluto_case "awweee":
        break
    pluto_default:
        error()
end
value = nil
pluto_switch (value) do
    pluto_case -1:
    pluto_case nil:
    pluto_case -2:
        break
    pluto_default:
        error()
end
value = -24389
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case 231923:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        break
    pluto_default: 
        error()
end
value = -1
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case 231923:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        error()
end
value = -3.14
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case -3.14:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        break
end
value = -3.3
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case -3.15:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        error()
end
t = 0
value = -3.15
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case -3.15:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        t = true
end
assert(t == true)

t = 0
value = -3.15
pluto_switch (value) do
    pluto_case "aawdkawmlwadmlaw":
    pluto_case "q49324932":
    pluto_case nil:
    pluto_case "130-91230921":
    pluto_case false:
    pluto_case -3.15:
    pluto_case true:
    pluto_case -234234:
    pluto_case -24389:
    pluto_case 23429:
    pluto_case "bar":
    pluto_case "foobar":
    pluto_case "barfoo":
        t = true
        break
    pluto_default:
        t = false
end
assert(t == true)

print "Testing table freezing."
t = table.freeze({ 1, 2, 3, "hello", "world" })
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

print "Testing standard library additions."
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
local t = string.split("hello world abc", " ")
assert(t[1] == "hello")
assert(t[2] == "world")
assert(t[3] == "abc")
local before, after = string.partition("hello.wor.ld", ".")
assert(before == "hello")
assert(after == "wor.ld")
local before, after = string.partition("hello.wor.ld", ".", true)
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

print "Testing compatibility."
local a = "Hi"
local t = {a}
assert(t[1] == "Hi")
local t = {["func"]=function(p1,p2)end}
assert(t["func"] ~= nil)

print "Finished."