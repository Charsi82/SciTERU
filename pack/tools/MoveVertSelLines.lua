--[[--------------------------------------------------
MoveVertSelLines.lua
Authors: Charsi
Version: 0.1
------------------------------------------------------
Description:
 Перемещение строк, содержащих выделение, вверх или вниз с сохранением выделения
------------------------------------------------------
Connection:
In file .properties add a lines:

   command.name.70.*=Upwards
   command.70.*=dostring up=true dofile(props["SciteDefaultHome"].."\\tools\\MoveVertSelLines.lua")
   command.mode.70.*=subsystem:lua,savebefore:no,clearbefore:no
   command.shortcut.70.*=Alt+Up

  command.name.71.*=Downwards
  command.71.*=dostring up=false dofile(props["SciteDefaultHome"].."\\tools\\MoveVertSelLines.lua")
  command.mode.71.*=subsystem:lua,savebefore:no,clearbefore:no
  command.shortcut.71.*=Alt+Down
--]] -------------------------------------------------
local _gebug = false

local selections = editor.SelectionSerialized
local info = _gebug and print or function() end

local function MoveLineUp(num)
	if num == 0 then return false end
	editor:GotoLine(num)
	editor:LineTranspose()
	editor:LineUp()
	return true
end

local function MoveLineDown(num)
	if num + 1 >= editor.LineCount then return false end
	return MoveLineUp(num + 1)
end

if editor.SelectionIsRectangle then
	-- R[anchor]
	-- R[anchor][v_anch]
	-- R[anchor][v_anch]-[caret][v_car]
	-- R#N,[anchor][v_anch]-[caret][v_car]

	local function make_Rselection(old_sel, new_anc, new_car, v_anch, scar, v_car)
		local cap = old_sel:match("^R#%d+,") or "R"
		local new_sel = cap .. new_anc
		if #v_anch > 0 then new_sel = new_sel .. 'v' .. v_anch end
		if #scar > 0 then
			new_sel = new_sel .. '-' .. new_car
			if #v_car > 0 then new_sel = new_sel .. 'v' .. v_car end
		end
		return new_sel
	end

	local sanc, v_anch, scar, v_car = selections:match("[R,](%d+)v?(%d*)%-?(%d*)v?(%d*)")
	local anc = tonumber(sanc)
	local car = tonumber(scar) or anc
	local al = editor:LineFromPosition(anc)
	local cl = editor:LineFromPosition(car)
	local from, to = math.min(al, cl), math.max(al, cl)
	if up then
		local len = editor:LineLength(from - 1)
		for i = from, to, 1 do if not MoveLineUp(i) then return end end
		-- move selection area
		local rsel = make_Rselection(selections, anc - len, car - len, v_anch, scar, v_car)
		ApplySelection(rsel)
	else
		local len = editor:LineLength(to + 1)
		for i = to, from, -1 do if not MoveLineDown(i) then return end end
		local _, n_eol = GetEOL()
		if to == editor.LineCount - 2 then len = len + n_eol end
		local rsel = make_Rselection(selections, anc + len, car + len, v_anch, scar, v_car)
		ApplySelection(rsel)
	end
else
	-- caret
	-- anchor-caret
	-- #num,anchor-caret,anchor-caret

	local function UpdateSelection(selections, blocks)
		table.sort(blocks, function(a, b) return a.id < b.id end)
		local nst = {}
		nst[1] = selections:match("(#%d+),") -- maybe nil!
		for k, v in ipairs(blocks) do
			nst[#nst+1] = (v[1] == v[2]) and v[1] or (v[1]..'-'.. v[2])
		end
		local ns = table.concat(nst, ',')
		ApplySelection(ns)
	end

	if up then
		local _, start_pos = selections:match("#(%d+)()")
		local blocks = {}
		for _start in selections:gmatch("[^,]+", start_pos) do
			local s_anchor, s_caret = _start:match("(%d+)%-?(%d*)")
			local anchor = tonumber(s_anchor)
			local caret = tonumber(s_caret) or anchor
			local _min = math.min(anchor, caret)
			local line_num1 = editor:LineFromPosition(_min)
			if line_num1 == 0 then
				info('on top')
				return
			end
			local _max = math.max(anchor, caret)
			local line_num2 = editor:LineFromPosition(_max)
			blocks[#blocks + 1] = {id = #blocks + 1, _min, _max, line_num1, line_num2}
		end
		table.sort(blocks, function(a, b) return a[3] < b[3] end)
		local mlines = {}
		for k, v in ipairs(blocks) do
			local ln = nil
			for i = v[3], v[4] do
				if not ln then ln = mlines[i - 1] or editor:LineLength(i - 1) end
				if not mlines[i] then mlines[i] = ln end
			end
			v[1] = v[1] - ln
			v[2] = v[2] - ln
		end

		-- move lines
		local _lines = {}
		for k in pairs(mlines) do _lines[#_lines + 1] = k end
		table.sort(_lines)
		for _, v in ipairs(_lines) do MoveLineUp(v) end

		-- build new selection
		UpdateSelection(selections, blocks)
	else
		local _, start_pos = selections:match("#(%d+)()")
		local blocks = {}
		for _start in selections:gmatch("[^,]+", start_pos) do
			local s_anchor, s_caret = _start:match("(%d+)%-?(%d*)")
			local anchor = tonumber(s_anchor)
			local caret = tonumber(s_caret) or anchor
			local _max = math.max(anchor, caret)
			local line_num2 = editor:LineFromPosition(_max)
			if line_num2 == editor.LineCount - 1 then
				info('on bottom')
				return
			end
			local _min = math.min(anchor, caret)
			local line_num1 = editor:LineFromPosition(_min)
			blocks[#blocks + 1] = {id = #blocks + 1, _min, _max, line_num1, line_num2}
		end
		table.sort(blocks, function(a, b) return a[4] > b[4] end)
		local mlines = {}
		local _, n_eol = GetEOL()
		for k, v in ipairs(blocks) do
			local ln = nil
			for i = v[4], v[3], -1 do
				if not ln then
					ln = mlines[i + 1] or editor:LineLength(i + 1)
					if i == editor.LineCount - 2 then ln = ln + n_eol end -- fix eol size
				end
				if not mlines[i] then mlines[i] = ln end
			end
			v[1] = v[1] + ln
			v[2] = v[2] + ln
		end
		
		-- move lines
		local _lines = {}
		for k in pairs(mlines) do _lines[#_lines + 1] = k end
		table.sort(_lines, function(a, b) return a > b end)
		for _, v in ipairs(_lines) do MoveLineDown(v) end
		
		-- build new selection
		UpdateSelection(selections, blocks)
	end
end
