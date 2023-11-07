-------------------------
-- '2048' the GAME
-------------------------
local nums = {}
local score = 0
local label_score = nil
local list = nil
local st = {}
local function reduce_num(...)
	local idxs = {...}
	local args = {}
	for i = 1, #idxs do args[i] = nums[idxs[i]] end
	local res = {}
	local last = 0
	local move = false
	for j=1,4 do
		if args[j]>0 then
			if args[j]~=last then
				last = args[j]
				res[#res+1] = last
				move = true
			else
				res[#res] = last + last
				score = score + last + last
				move = true
				last = 0
			end
		end
	end
	for i=1, #idxs do nums[idxs[i]] = res[i] or 0 end
	-- print(table_to_string(args))
	-- print( table.concat(nums, ',') )
	return move
end
local last_idx = 0
local function add_item()
	local t = {}
	for i = 1,16 do if nums[i]==0 then t[#t+1] = i end end
	if #t==0 then return false end
	last_idx = math.random(#t)
	--print('add_item', last_idx, 'from', #t)
	nums[last_idx] = (math.random()<0.7) and 2 or 4
	return true
end

local function update_ui()
	list:clear()
	for i = 1, 16 do
		list:add_item(nums[i], nil, math.floor(math.log(nums[i]+1)/math.log(2))) --caption, data, icon_idx
	end
	list:set_selected_item(last_idx-1)
	-- print(last_idx)
	label_score:set_text('score:' .. score)
end

local function on_key(key)
	-- print('on_key', key)
	--[[
	  1  2  3  4
	  5  6  7  8
	  9 10 11 12
	 13 14 15 16
	]]
	local need_update = false
	if key==40 then
		-- print('down')
		need_update = reduce_num(13,9,5,1) or need_update
		need_update = reduce_num(14,10,6,2) or need_update
		need_update = reduce_num(15,11,7,3) or need_update
		need_update = reduce_num(16,12,8,4) or need_update
	elseif key == 39 then
		-- print('right')
		need_update = reduce_num(4,3,2,1) or need_update
		need_update = reduce_num(8,7,6,5) or need_update
		need_update = reduce_num(12,11,10,9) or need_update
		need_update = reduce_num(16,15,14,13) or need_update
	elseif key == 38 then
		-- print('up')
		need_update = reduce_num(1,5,9,13) or need_update
		need_update = reduce_num(2,6,10,14) or need_update
		need_update = reduce_num(3,7,11,15) or need_update
		need_update = reduce_num(4,8,12,16) or need_update
	elseif key == 37 then
		-- print('left')
		need_update = reduce_num(1,2,3,4) or need_update
		need_update = reduce_num(5,6,7,8) or need_update
		need_update = reduce_num(9,10,11,12) or need_update
		need_update = reduce_num(13,14,15,16) or need_update
	end
	if need_update then
		add_item()
		update_ui()
	end
end

return function(tabs, panel_width, colorback, colorfore)
	local tab = gui.panel(panel_width)
	--------------------------------
	list = tab:add_list( _, _, true)
	list:set_iconlib([[%windir%\system32\shell32.dll]], false)
	list:position(10, 10)
	list:size(3, 3)
	tab:on_key(function(key) print('on_key', key) end)
	
	label_score = tab:add_label(0x1)
	label_score:position(10, 315)
	label_score:size(50, 20)
	--------------------------------
	local function reset()
		for i = 1, 16 do nums[i] = 0 end
		score = 0
		last_idx = math.random(16)
		nums[last_idx] = math.random()<0.7 and 2 or 4
		-- print(last_idx)
		update_ui()
	end
	
	----------------------------
	local size = 70
	for i=1, 16 do
		st[i] = tab:add_button("none", 0, false, 0x8000+ 0x1)
		st[i]:set_text(i)
		st[i]:enable(false)
		st[i]:position(5 + (i-1)%4*size, 35 + (i-1)//4*size)
		st[i]:size(size, size)
	end
	----------------------------
	
	local btn_new = tab:add_button('New', 1)
	btn_new:position(10, 340)
	btn_new:size(50, 20)
	tab:on_command(function(id)
		if id == 1 then
			reset()
		end
	end)
	--------------------------------
	reset()
	tabs:add_tab("2048", tab, 37) -- caption, wnd, icon_index
	return tab
end