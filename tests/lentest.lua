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
print("Finished test.")