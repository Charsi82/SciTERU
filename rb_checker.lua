-- RB checker of changes
local path = debug.getinfo(1).source:match("@?(.*\\)") --[[E:\scite555]]
local res = {}
local start_pos = #path + 2
local function printt(t) print(table.concat(t, ',')) end
local function printst(t)
	local keys = {}
	for k, v in pairs(t) do table.insert(keys, k) end
	table.sort(keys)
	for _, key in pairs(keys) do print(key, t[key]) end
end
local function printfst(t)
	local keys = {}
	for k, v in pairs(t) do table.insert(keys, k) end
	table.sort(keys)
	for _, key in pairs(keys) do io.write(key:sub(start_pos), " ", t[key], "\n") end
end
local function process_file(fn)
	if string.find(fn, "%.h$") or string.find(fn, "%.cxx$") or string.find(fn, "%.cpp$") then
		-- print('fn=', fn)
		local file = io.open(fn)
		if file then
			for line in file:lines() do if line:find("ifn?def RB_") then res[fn] = (res[fn] or 0) + 1 end end
			file:close()
		else
			print("can't open file:", fn)
		end
	end
end

function process_dir(dir)
	-- print("process_dir", dir)
	local files = gui.files(dir .. "\\*", false)
	for i, filename in ipairs(files) do process_file(dir .. "\\" .. filename) end
	local dirs = gui.files(dir .. "\\*", true)
	for i, subdir in ipairs(dirs) do process_dir(dir .. "\\" .. subdir) end
	-- for i, subdir in ipairs(dirs) do print(dir .. "\\" .. subdir) end
	-- printt(dirs)
end
process_dir(path)
-- printst(res)
local total = 0
for k, v in pairs(res) do total = total + v end
print('total =', total)
io.output(path .. "rb_checker_out.txt")
io.write(path, '\n')
printfst(res)
io.write('total = ', total)
io.close()
