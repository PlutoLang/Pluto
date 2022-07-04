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
switch (value) do
    case 5:
        break
    default:
        error()
end
value = 3
switch value do
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        break
    default:
        error()
end
value = "foo"
switch (value) do
    case "foo":
        break
    default:
        error()
end
switch (value) do
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
switch (value) do
    case -1:
    case nil:
    case -2:
        break
    default:
        error()
end
value = -24389
switch (value) do
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
switch (value) do
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
switch (value) do
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
switch (value) do
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
switch (value) do
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
switch (value) do
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
assert(table.contains(t, "Hello") == true)
assert(table.contains(t, "World") == false)
assert(table.contains(t, true) == true)
assert(table.contains(t, false) == false)
assert(table.contains(t, 0) == true)
assert(table.contains(t, 1) == false)

print "Finished."