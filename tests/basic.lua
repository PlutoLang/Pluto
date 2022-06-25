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