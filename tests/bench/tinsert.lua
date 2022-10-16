local t

t = os.clock()
do
	local b = {}
	for i = 1, 1000000 do
		table.insert(b, "hello")
	end
end
t = os.clock() - t
print(t)

t = os.clock()
do
	local b = {}
	for i = 1, 1000000 do
		b[#b + 1] = "hello"
	end
end
t = os.clock() - t
print(t)