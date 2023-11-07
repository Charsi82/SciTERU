--[[
Restore bookmark, folds, caret position 
]]
----------------------------------------------------------------
local LifeTime = 60*60*24*7 -- 7 days to die
local path = props["SciteUserHome"].."\\RecentStorage.lua"
----------------------------------------------------------------
RecentData = {}
local isOk, res = pcall(dofile, path)
if isOk and res and type(res)=='table' then RecentData = res else print('loading recent.lua failed') end

for i=#RecentData,1,-1 do
	if os.time()-RecentData[i].timestamp > LifeTime then
		table.remove(RecentData, i)
	end
end

function table_to_string(t, indent)
	if not indent then indent="" end
	local res = indent.."{"
	for k, v in pairs(t) do
		if type(k)=='number' then
			if type(v)=='table' then
				res = res..string.format("\n%s[%s] =\n%s,", indent, k, table_to_string(v, indent.."\t"))
			else
				res = res..string.format("\n%s[%s] = %q,", indent, k, v)
			end
		else
			if type(v)=='table' then
				res = res..string.format("\n%s['%s'] =\n%s,", indent, k, table_to_string(v, indent.."\t"))
			else
				res = res..string.format("\n%s['%s'] = %q,", indent, k, v)
			end
		end
	end
	return res.."\n"..indent.."}"
end

AddEventHandler("OnFinalise", function()
	local f = io.open(path, "wb")
	if f then
		f:write("return \n"..table_to_string(RecentData))
		f:close()
	end
end)

local function Restore(file)
-- 	print('restore data for:', file)
	local item = 0
	for i=1,#RecentData do
		if RecentData[i].path == file then item = i break end
	end
	if item == 0 then
-- 		print('data for item', file, 'not found')
		local toggle_foldall_ext = props['fold.on.open.ext']:lower()
		local current_ext = props['FileExt']:lower()
		for ext in toggle_foldall_ext:gmatch("%w+") do
			if current_ext == ext then scite.MenuCommand (IDM_TOGGLE_FOLDALL) break end
		end
		return
	end
	
	-- folding
-- 	print('restore folding for', file)
	if tonumber(props['session.folds']) == 1 then
		for _, line in pairs (RecentData[item].folds or {}) do
-- 			print('reset fold', line)
			editor:FoldLine(line, 0)
		end
	end
	
	-- bookmarks
-- 	print('restore bookmarks for', file)
	if tonumber(props['session.bookmarks']) == 1 then
		for k, v in pairs (RecentData[item].bookmarks or {}) do
-- 			print('reset bookmark', v.line)
			if editor:MarkerGet(v.line, 2)//2%2~=1 then
				editor:MarkerAdd(v.line, 1)
			end
		end
	end
	
	-- position
-- 	print('restore position for', file)
	if tonumber(props['save.position']) == 1 then
		local pos = tonumber(RecentData[item].caret) or 0
		editor:GotoPos(pos)
	end
end

AddEventHandler("OnOpen", function(file)
	if file ~= '' and tonumber(props['save.session']) == 1 then
		event("OnUpdateUI"):register(function(e)
			e:removeThisCallback()
			Restore(PathCollapse(file))
			-- print('rrecent: upd ui')
		end)
	end
end)

local function GetLineText(line_number)
	local ELLIPSIS_LEN = 30
	local line_text = editor:GetLine(line_number) or ""
	line_text = line_text:gsub('^%s+', ''):gsub('%s+', ' ')
	if line_text == '' then
		line_text = ' - empty line'
	end
	line_text = "["..(line_number+1).."] "..line_text
	if #line_text > ELLIPSIS_LEN then line_text = line_text:sub(1, ELLIPSIS_LEN-3).."..." end
	return line_text
end

AddEventHandler("OnClose", function(file)
	if file=='' then return end
	-- gui.log("OnClose:"..file)
	local function get_bookmarks()
		local res = {}
		local line = 0
		while true do
			line = editor:MarkerNext(line, 2)
			if (line == -1) then break end
			res[#res+1] = {
			line = line,
			caption = GetLineText(line)
			}
			line = line + 1
		end
		return res
	end
	
	local function get_folds()
		local res = {}
		local line = 0
		while true do
			line = editor:ContractedFoldNext(line)
			if (line == -1) then break end
			res[#res+1] = line
			line = line + 1
		end
		return res
	end
	
	local pc = PathCollapse(file)
	local item = 0
	for i=1,#RecentData do
		if RecentData[i].path == pc then item = i break end
	end
	RecentData[(item == 0) and (#RecentData+1) or item] =
	{
		timestamp = os.time(),
		path = pc,
		bookmarks = get_bookmarks(),
		folds = get_folds(),
		caret = editor.CurrentPos
	}
end)

