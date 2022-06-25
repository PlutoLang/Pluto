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

print "Finished."