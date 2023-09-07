local t = {}
for i = 1, 1000 do
  t[i] = i
end

local start = os.clock()
local cc = 0
for i, _ in ipairs(t) do
  for j, _ in ipairs(t) do
    for k, _ in ipairs(t) do
      if i + j == k then
        cc = cc + 1
      end
    end
  end
end
print(os.clock() - start)
return print(cc)