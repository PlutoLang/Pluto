local start = os.clock()
local cc = 0
for i = 1, 1000 do
  for j = 1, 1000 do
    for k = 1, 1000 do
      if i + j == k then
        cc = cc + 1
      end
    end
  end
end
print(os.clock() - start)
return print(cc)