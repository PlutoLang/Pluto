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

local str = "a"
str ..= "b"
assert(str == "ab")
str ..= "c"
assert(str == "abc")

assert("ab".."c" == "abc")

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

print "Testing pluto_switch statement."
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

print "Finished."