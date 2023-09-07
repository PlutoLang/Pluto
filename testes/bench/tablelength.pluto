local t = {}
for i = 1, 100000 do
    t[i - 2] = "aadw"
    t[i + 1] = 444
end

local s = os.clock()
for i = 1, 100000000 do
    local len = #t
end
print(os.clock() - s)