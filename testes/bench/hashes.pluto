local crypto = require("crypto")
local f = io.open("tests/bench/sherlock.txt")
local text = f:read("*all")
local str = "Cats are interesting."
local str1 = "a"
local strt = { 
	str1:rep(4),
 	str1:rep(16),
 	str1:rep(32),
 	str1:rep(32) .. str1 .. str1:rep(4),
	str:rep(256),
	"",
	"dupa",
	str,
	str1,
	text
}

local testIndex   = 10
local testRepeat  = 10
local hashRepeat  = 1000

local function test(f)
	local times       = {}
	local throughputs = {}

	local len = #strt[testIndex]
	for i=1,testRepeat do
		local start = os.clock()
		for i=1,hashRepeat do f(strt[testIndex], len) end
		local time = os.clock() - start
		local throughput = len*hashRepeat/time/(1024*1024*1024)
		table.insert(times, time)
		table.insert(throughputs, throughput)
	end

	print( ("\tmin: %.3fs %.2f GB/s"):format( math.min(table.unpack(times)), math.min(table.unpack(throughputs))) )
	print( ("\tmax: %.3fs %.2f GB/s"):format( math.max(table.unpack(times)), math.max(table.unpack(throughputs))) )
end


local forbiddedTests = {
	random = 0;
	hexdigest = 0;
}


for key, value in crypto do
	if not (key in forbiddedTests) then
		print("Benchmarking: '" .. key .. "'")
		test(value)
	end
end