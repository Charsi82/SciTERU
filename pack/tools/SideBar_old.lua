--[[--------------------------------------------------
SideBar.lua
Authors: Frank Wunderlich, mozers™, VladVRO, frs, BioInfo, Tymur Gubayev, ur4ltz, nicksabaka
Version 1.28.2
------------------------------------------------------
  Note: Require gui.dll
               lpeg.dll
              shell.dll
             COMMON.lua (function GetCurrentWord)
             eventmanager.lua (function AddEventHandler)

  Connection:
   In file SciTEStartup.lua add a line:
      dofile (props["SciteDefaultHome"].."\\tools\\SideBar.lua")

   Set in a file .properties:
    command.checked.17.*=$(sidebar.show)
    command.name.17.*=SideBar
    command.17.*=SideBar_ShowHide
    command.mode.17.*=subsystem:lua,savebefore:no

    # Set window(1) or pane(0) (default = 0)
    sidebar.win=1

    # Set show(1) or hide(0) to SciTE start
    sidebar.show=1

    # Set sidebar width and position
    sidebar.width=250 (default = 230)
    sidebar.position=left or right (default = right)

    # Set default settings for Functions/Procedures List
    sidebar.functions.flags=1
    sidebar.functions.params=1

    # Set abbrev preview 1-calltip, 0-annotation(default)
    sidebar.abbrev.calltip=1

    # Set annotation style
    style.*.255=fore:#808080,back:#FEFEFE
--]]--------------------------------------------------
require 'gui'
require 'lpeg'
require 'shell'

-- local _DEBUG = true --включает вывод отладочной информации

-- you can choose to make SideBar a stand-alone window
local win = tonumber(props['sidebar.win']) == 1

-- Переключатель способа предпросмотра аббревиатур: true = calltip, false = annotation
local Abbreviations_USECALLTIPS = tonumber(props['sidebar.abbrev.calltip']) == 1
-- отображение флагов/параметров по умолчанию:
local _show_flags = tonumber(props['sidebar.functions.flags']) == 1
local _show_params = tonumber(props['sidebar.functions.params']) == 1

local tab_index = 0
local panel_width = tonumber(props['sidebar.width']) or 326
local win_height = tonumber(props['position.height']) or 600
local sidebar_position = (props['sidebar.position'] == 'left') and 'left' or 'right'
props['sidebar.position'] = sidebar_position
local style = props['style.*.32']
local colorback = style:match('back:(#%x%x%x%x%x%x)')
local colorfore = style:match('fore:(#%x%x%x%x%x%x)') or '#000000'

----------------------------------------------------------
-- Common functions
----------------------------------------------------------

local function ReplaceWithoutCase(text, s_find, s_rep)
	s_find = string.gsub(s_find, '.', function(ch)
		if ch:match('%d') then return ch end -- avoid %d pattern
		local c,C = ch:lower(), ch:upper()
		return c~=C and '['..c..C..']' -- i.e. ch was a letter
				or '%'..ch -- not a letter, so escape it
	end)
	return string.gsub(text, s_find, s_rep)
end

local function ShowCompactedLine(line_num)
	local function GetFoldLine(ln)
		while editor.FoldExpanded[ln] do ln = ln-1 end
		return ln
	end
	while not editor.LineVisible[line_num] do
		local x = GetFoldLine(line_num)
		editor:ToggleFold(x)
		line_num = x - 1
	end
end

if _DEBUG then
local nametotime = {} -- maps names to starttimes
	_DEBUG = {}

	_DEBUG.timerstart = function (name)
		nametotime[name] = os.clock()
	end -- _DEBUG.timerstart

	_DEBUG.timer = function (name,...)
		if nametotime[name] then
			local d = os.clock() - nametotime[name]
			print(name,('%.5fs'):format(d),...)
			return d
		end
	end -- _DEBUG.timer

	_DEBUG.timerstop = function (name,...)
		local d = _DEBUG.timer(name,...)
		nametotime[name] = nil
		return d
	end --_DEBUG.timerstop

else
	_DEBUG = {}
	local empty = function (...) end
	_DEBUG.timer, _DEBUG.timerstart, _DEBUG.timerstop = empty, empty, empty
end

----------------------------------------------------------
-- Create panels
----------------------------------------------------------
local win_parent
if win then
	win_parent = gui.window "Side Bar"
	win_parent:size(panel_width + 24, 600)
	win_parent:on_close(function() props['sidebar.show']=0 end)
else
	win_parent = gui.panel(panel_width)
end

-------------------------
local tab0 = gui.panel(panel_width)

local memo_path = gui.memo()
tab0:add(memo_path, "top", 22)
if colorback then memo_path:set_memo_colour('', colorback) end

local list_dir_height = win_height/3
if list_dir_height <= 0 then list_dir_height = 200 end
local list_favorites = gui.list(true)
list_favorites:add_column("Избранное", panel_width)
tab0:add(list_favorites, "bottom", list_dir_height)
if colorback then list_favorites:set_list_colour(colorfore,colorback) end

local list_dir = gui.list(false, false) -- MultiColumn = false, SingleSelect = true
tab0:client(list_dir)
if colorback then list_dir:set_list_colour(colorfore,colorback) end

-------------------------
local tab1 = gui.panel(panel_width)

local list_func_height = win_height/3
if list_func_height <= 0 then list_func_height = 200 end
local list_bookmarks = gui.list(true)
list_bookmarks:add_column("#", 24)
list_bookmarks:add_column("Закладки", panel_width)
tab1:add(list_bookmarks, "bottom", list_func_height)
if colorback then list_bookmarks:set_list_colour(colorfore,colorback) end

local list_func = gui.list(true)
list_func:add_column("Функции/Процедуры", 600)
tab1:client(list_func)
if colorback then list_func:set_list_colour(colorfore,colorback) end

-------------------------
local tab2 = gui.panel(panel_width)

local list_abbrev = gui.list(true)
list_abbrev:add_column("Сокращения", 60)
list_abbrev:add_column("Расширение", 600)
tab2:client(list_abbrev)
if colorback then list_abbrev:set_list_colour(colorfore,colorback) end

-------------------------
local tab3 = gui.panel(panel_width)
local list_ascii= gui.list(true, false)
list_ascii:add_column("DEC", 20)
list_ascii:add_column("HEX", 20)
list_ascii:add_column("Символ", 20)
list_ascii:add_column("HTML Номер", 20)
list_ascii:add_column("HTML Код", 20)
tab3:client(list_ascii)
if colorback then list_ascii:set_list_colour(colorfore,colorback) end

-------------------------
-- local tab_demo = true
-- if tab_demo then
local tab4 = gui.panel(panel_width)
-------------------
local tree = gui.tree(true,false) -- haslines[true], editable[false]
-- tab4:client(tree)
tab4:add(tree,'top',150)
-- local itm1 = tree:add_item("item1", nil, 1) -- ("caption" [, parent_item = root][, icon_idx=-1][, sel_icon_idx=-1])
--[[tree:add_item("item2", itm1, 1)
tree:add_item("item3")
local itm4 = tree:add_item("item4")
local itm5 = tree:add_item("item5", itm4,2)
local itm6 = tree:add_item("item6", itm5,3)
local itm7 = tree:add_item("item7", itm6,4)]]
-- tree:set_iconlib()
-- local imgcnt = tree:set_iconlib([[toolbar\cool.dll]]) --path [, small_size = true] return count of loaded icons
-- local imgcnt = tree:set_iconlib([[%windir%\system32\shell32.dll]], false ) 
-- local imgcnt = tree:set_iconlib([[%SystemRoot%\system32\shell32.dll]])
--[[tree:add_item("qwerty", nil, 1)
tree:add_item("qwerty2", nil, 1)
tree:add_item("qwerty3", nil, 1)]]

local imgcnt = tree:set_iconlib([[toolbar\cool.dll]]) --path [, small_size = true] return count of loaded icons
local lib1_itm = tree:add_item("qwerty", nil, 4)
for icon_idx=0, imgcnt-1 do
	tree:add_item("item_"..icon_idx, lib1_itm, icon_idx)
end


-- tree:set_selected_item(itm5)
-- print("test:",tree:tree_item_get_text(itm5))
tree:on_select(function(sel_item)
	print('tree_on_select2:', tree:tree_get_item_text(sel_item))
end)

function tree_test_menu() print('tree test') end --ok
tree:context_menu{"item1|tree_test_menu"} -- ok
-------------------
local listbox = gui.listbox(77)
tab4:add(listbox, "top", 100)
for i=1,15 do
	listbox:append("line"..i, i*i)
end
-- listbox:insert(idx, "caption"[,data]) -- ok
-- listbox:select(10) -- ok
listbox:remove(2) --ok
-- print("lb1:",listbox:count())
-- listbox:clear() --ok
-- print("lb2:",listbox:count()) -- ok
-- print("select=",listbox:select()) --ok
-- print("line_0_text=",listbox:get_line_text(4))
-- print("line_0_data=",listbox:get_line_data(4))
--[[listbox:on_select(function(...)
	print('istbox_select', ...)
end)]]
tab4:on_command(function(fromId, state)
-- print("tab4cmd:",fromId, state)
if fromId==77 then--[[
1 - clicked
2 - db clicked
4 - focused
5 - unfocused
]]
if state==2 then
-- 	print("db clicked list on ",listbox:get_line_text(listbox:select()))
end
end
end)
-------------------
local test_panel = gui.panel(panel_width)
tab4:add(test_panel, "top", 120)

local btn = gui.button("btn1",11)
test_panel:add(btn,"none")
btn:position(20,20)
btn:size(50,30)

local btn2 = gui.button("btn2",22, true)
test_panel:add(btn2,"none")
btn2:position(80,20)
btn2:size(50,30)

local chbox = gui.checkbox("check3",33, true) -- caption, id, auto
test_panel:add(chbox,"none")
chbox:position(140,20)

local rbtn = gui.radiobutton("radio_1",44, true) -- caption, id, auto
test_panel:add(rbtn,"none")
rbtn:position(20,60)
rbtn:check(true)

local rbtn2 = gui.radiobutton("radio_2",55, true)
test_panel:add(rbtn2,"none")
rbtn2:position(100,60)

local label1 = gui.label("StaticText",66)
test_panel:add(label1,"none")
label1:position(180,60)

--[[ toolbar style option
#define TBS_AUTOTICKS           0x0001
#define TBS_VERT                0x0002
#define TBS_HORZ                0x0000
#define TBS_TOP                 0x0004
#define TBS_BOTTOM              0x0000
#define TBS_LEFT                0x0004
#define TBS_RIGHT               0x0000
#define TBS_BOTH                0x0008
#define TBS_NOTICKS             0x0010
#define TBS_ENABLESELRANGE      0x0020
#define TBS_FIXEDLENGTH         0x0040
#define TBS_NOTHUMB             0x0080
#define TBS_TOOLTIPS            0x0100
#define TBS_REVERSED            0x0200
]]
local trbar1 = gui.trackbar(0x20 + 0x200, 77) -- style, id
test_panel:add(trbar1,"none")
trbar1:position(20,90)
trbar1:size(250, 20)
-- trbar1:size(20, 250) -- vert
-- trbar1:range(20,80)
--[[
-- style TBS_ENABLESELRANGE required
local selmin, selmax = trbar1:select() -- get selmin, selmax
trbar1:select(20,80) -- set selection
trbar1:sel_clear() -- clear selection
]]

test_panel:on_command(function(id)
	print("try command "..id)
	if id==11 then
		trbar1:select(30,60)
		--trbar1:set_pos(trbar1:get_pos()+5)
	-- 	print(">>",chbox:state(),chbox:check(),"<<")
	-- 	chbox:check( not chbox:check() )
		--rbtn:check( not rbtn:check() )
	end
	if id==22 then
		trbar1:sel_clear()
		print(trbar1:select())
		--print('pos = ',trbar1:get_pos())
	end
	return true
end)

test_panel:on_scroll( function(id, pos) label1:set_text("scroll pos: " .. pos) end)
-- end
-------------------------
local tabs = gui.tabbar(win_parent)
if shell.fileexists(props['sidebar.iconed.tabs']) then
	tabs:set_iconlib(props['sidebar.iconed.tabs'])
end
tabs:add_tab("Проводник", tab0, 57)
tabs:add_tab("Функции", tab1, 48)
tabs:add_tab("Сокращения", tab2, 22)
tabs:add_tab("ASCII", tab3, 21)

-- if tab_demo then
tabs:add_tab("Test", tab4, 37)
win_parent:client(tab4)
-- end

win_parent:client(tab3)
win_parent:client(tab2)
win_parent:client(tab1)
win_parent:client(tab0)

----------------------------------------------------------
-- tab0:memo_path   Path and Mask
----------------------------------------------------------
local current_path = ''
local file_mask = '*.*'

local function FileMan_ShowPath()
	local rtf = [[{\rtf{\fonttbl{\f0\fcharset204 Helv;}}{\colortbl;\red0\green0\blue255;\red255\green0\blue0;}\f0\fs16]]
	local path = '\\cf1'..current_path:gsub('\\', '\\\\')
	local mask = '\\cf2'..file_mask..'}'
	memo_path:set_text(rtf..path..mask)
end

memo_path:on_key(function(key)
	if key == 13 then
		local new_path = memo_path:get_text()
		if new_path ~= '' then
			new_path = new_path:match('[^*]+')..'\\'
			local is_folder = gui.files(new_path..'*', true)
			if is_folder then
				current_path = new_path
			end
		end
		FileMan_ListFILL()
		return true
	end
end)

----------------------------------------------------------
-- tab0:list_dir   File Manager
----------------------------------------------------------
function FileMan_ListFILL()
	if current_path == '' then return end
	local folders = gui.files(current_path..'*', true)
	if not folders then return end
	local table_folders = {}
	for i, d in ipairs(folders) do
		table_folders[i] = {'['..d..']', {d,'d'}}
	end
	table.sort(table_folders, function(a, b) return a[1]:lower() < b[1]:lower() end)
	local files = gui.files(current_path..file_mask)
	local table_files = {}
	if files then
		for i, filename in ipairs(files) do
			table_files[i] = {filename, {filename}}
		end
	end
	table.sort(table_files, function(a, b) return a[1]:lower() < b[1]:lower() end)

	list_dir:clear()
	list_dir:add_item ('[..]', {'..','d'})
	for i = 1, #table_folders do
		list_dir:add_item(table_folders[i][1], table_folders[i][2])
	end
	for i = 1, #table_files do
		list_dir:add_item(table_files[i][1], table_files[i][2])
	end
	list_dir:set_selected_item(0)
	FileMan_ShowPath()
	list_dir:autosize(0)
end

local function FileMan_GetSelectedItem(idx)
	if idx == nil then idx = list_dir:get_selected_item() end
	if idx == -1 then return '' end
	local data = list_dir:get_item_data(idx)
	local dir_or_file = data[1]
	local attr = data[2]
	return dir_or_file, attr
end

function FileMan_ChangeDir()
	local newPath = gui.select_dir_dlg('Выберите папку', current_path)
	if newPath == nil then return end
	if newPath:match('[\\/]$') then
		current_path = newPath
	else
		current_path = newPath..'\\'
	end
	FileMan_ListFILL()
end

function FileMan_MaskAllFiles()
	file_mask = '*.*'
	FileMan_ListFILL()
end

function FileMan_MaskOnlyCurrentExt()
	local filename, attr = FileMan_GetSelectedItem()
	if filename == '' then return end
	if attr == 'd' then return end
	file_mask = '*.'..filename:gsub('.+%.','')
	FileMan_ListFILL()
end

function FileMan_FileCopy()
	local filename = FileMan_GetSelectedItem()
	if filename == '' or filename == '..' then return end
	local path_destination = gui.select_dir_dlg("Копировать в...")
	if path_destination == nil then return end
	os_copy(current_path..filename, path_destination..'\\'..filename)
	FileMan_ListFILL()
end

function FileMan_FileMove()
	local filename = FileMan_GetSelectedItem()
	if filename == '' or filename == '..' then return end
	local path_destination = gui.select_dir_dlg("Переместить...")
	if path_destination == nil then return end
	os.rename(current_path..filename, path_destination..'\\'..filename)
	FileMan_ListFILL()
end

function FileMan_FileRename()
	local filename = FileMan_GetSelectedItem()
	if filename == '' or filename == '..' then return end
	local filename_new = shell.inputbox("Переименовать", "Новое имя файла:", filename, function(name) return not name:match('[\\/:|*?"<>]') end)
	if filename_new == nil then return end
	if filename_new ~= '' and filename_new ~= filename then
		os.rename(current_path..filename, current_path..filename_new)
		FileMan_ListFILL()
	end
end

function FileMan_FileDelete()
	local filename, attr = FileMan_GetSelectedItem()
	if filename == '' then return end
	if attr == 'd' then return end
	if shell.msgbox("Уверены, что хотите УДАЛИТЬ этот файл?\n"..filename, "DELETE", 4+256) == 6 then
	-- if gui.message("Are you sure you want to DELETE this file?\n"..filename, "query") then
		os.remove(current_path..filename)
		FileMan_ListFILL()
	end
end

local function FileMan_FileExecWithSciTE(cmd, mode)
	local p0 = props["command.0.*"]
	local p1 = props["command.mode.0.*"]
	local p2 = props["command.name.0.*"]
	props["command.name.0.*"] = 'tmp'
	props["command.0.*"] = cmd
	if mode == nil then mode = 'console' end
	props["command.mode.0.*"] = 'subsystem:'..mode..',savebefore:no'
	scite.MenuCommand(9000)
	props["command.0.*"] = p0
	props["command.mode.0.*"] = p1
	props["command.name.0.*"] = p2
end

function FileMan_FileExec(params)
	if params == nil then params = '' end
	local filename = FileMan_GetSelectedItem()
	if filename == '' then return end
	local file_ext = filename:match("[^.]+$")
	if file_ext == nil then return end
	file_ext = '%*%.'..string.lower(file_ext)
	
	local function CommandBuild(lng)
		local cmd = props['command.build.$(file.patterns.'..lng..')']
		cmd = cmd:gsub(props["FilePath"], current_path..filename)
		return cmd
	end
	-- Lua
	if string.match(props['file.patterns.lua'], file_ext) then
		dostring(params)
		dofile(current_path..filename)
	-- Batch
	elseif string.match(props['file.patterns.batch'], file_ext) then
		FileMan_FileExecWithSciTE(CommandBuild('batch'))
		return
	-- WSH
	elseif string.match(props['file.patterns.wscript']..props['file.patterns.wsh'], file_ext) then
		FileMan_FileExecWithSciTE(CommandBuild('wscript'))
	-- Other
	else
		local ret, descr = shell.exec(current_path..filename..params)
		if not ret then
			print (">Exec: "..filename)
			print ("Error: "..descr)
		end
	end
end

function FileMan_FileExecWithParams()
	if scite.ShowParametersDialog('Выполнение "'..FileMan_GetSelectedItem()..'". Установите папаметры:') then
		local params = ''
		for p = 1, 4 do
			local ps = props[tostring(p)]
			if ps ~= '' then
				params = params..' '..ps
			end
		end
		FileMan_FileExec(params)
	end
end

local function OpenFile(filename)
	if filename:match(".session$") ~= nil then
		filename = filename:gsub('\\','\\\\')
		scite.Perform ("loadsession:"..filename)
	else
		scite.Open(filename)
	end
	gui.pass_focus()
end

local function FileMan_OpenItem()
	local dir_or_file, attr = FileMan_GetSelectedItem()
	if dir_or_file == '' then return end
	if attr == 'd' then
		gui.chdir(dir_or_file)
		if dir_or_file == '..' then
			local new_path = current_path:gsub('(.*\\).*\\$', '%1')
			if not gui.files(new_path..'*',true) then return end
			current_path = new_path
		else
			current_path = current_path..dir_or_file..'\\'
		end
		FileMan_ListFILL()
	else
		OpenFile(current_path..dir_or_file)
	end
end

function FileMan_OpenSelectedItems()
	local si = list_dir:get_selected_items()
	for _,i in ipairs(si) do
		local dir_or_file, attr = FileMan_GetSelectedItem(i)
		if attr ~= 'd' then
			OpenFile(current_path..dir_or_file)
		end
	end
end

function FileMan_SelectCurrentFile()
	local path = props['FileDir']
	if path == '' then return end
	current_path = path:gsub('\\$','')..'\\'
	file_mask = '*.*'
	FileMan_ListFILL()
	local fn = props['FileNameExt']
	local cnt = list_dir:count()
	list_dir:set_selected_item(-1) -- clear selection
	for i=1,cnt-1 do
		if fn == list_dir:get_item_text(i) then list_dir:set_selected_item(i) return end
	end
end

function FileMan_Explore()
	local dir_or_file, attr = FileMan_GetSelectedItem()
	local cmd = ""
	if attr == 'd' then
		if dir_or_file==".." then dir_or_file = "" else cmd = "\\" end
	end
	shell.exec("explorer /select,"..current_path..dir_or_file..cmd)
end

list_dir:on_double_click(function()
	FileMan_OpenItem()
end)

list_dir:on_key(function(key)
	if key == 13 then -- Enter
		if list_dir:selected_count() > 1 then
			FileMan_OpenSelectedItems()
		else
			FileMan_OpenItem()
		end
	elseif key == 8 then -- BackSpace
		list_dir:set_selected_item(0)
		FileMan_OpenItem()
	elseif key == 46 then -- Delete
		FileMan_FileDelete()
	elseif key == 45 then -- Insert
		Favorites_AddFile()
	end
end)

----------------------------------------------------------
-- tab0:list_favorites   Favorites
----------------------------------------------------------
local favorites_filename = props['SciteUserHome']..'\\favorites.lst'
local list_fav_table = {}

local function Favorites_ListFILL()
	list_favorites:clear()
	table.sort(list_fav_table,
		function(a, b)
			local function IsSession(filepath)
				return filepath:gsub('^.*%.',''):upper() == 'SESSION'
			end
			local isAses = IsSession(a[1]:lower())
			local isBses = IsSession(b[1]:lower())
			if (isAses and isBses) or not (isAses or isBses) then
				return a[1]:lower() < b[1]:lower()
			else
				return isAses
			end
		end
	)
	for _, file in ipairs(list_fav_table) do
		list_favorites:add_item(file[1], file[2])
	end
end

local function Favorites_OpenList()
	local favorites_file = io.open(favorites_filename)
	if favorites_file then
		for fpath in favorites_file:lines() do
			if fpath ~= '' then
				fpath = ReplaceWithoutCase(fpath, '$(SciteDefaultHome)', props['SciteDefaultHome'])
				local fname = fpath:gsub('.+\\','')
				if fname == '' then fname = fpath:gsub('.+\\(.-)\\',' [%1]') end
				list_fav_table[#list_fav_table+1] = {fname, fpath}
			end
		end
		favorites_file:close()
	end
	Favorites_ListFILL()
end
Favorites_OpenList()

local function Favorites_SaveList()
	if pcall(io.output, favorites_filename) then
		for _, file in ipairs(list_fav_table) do
			io.write(ReplaceWithoutCase(file[2], props['SciteDefaultHome'], '$(SciteDefaultHome)')..'\n')
		end
		io.close()
	end
end

local function Favorites_ADD(fname, fpath)
	for _, t in ipairs(list_fav_table) do
		if t[1] == fname and t[2] == fpath then return end
	end
	list_fav_table[#list_fav_table+1] = {fname, fpath}
end

function Favorites_AddFile()
	local fname, attr = FileMan_GetSelectedItem()
	if fname == '' then return end
	local fpath = current_path..fname
	if attr == 'd' then
		fname = ' ['..fname..']'
		fpath = fpath:gsub('\\%.%.$', '')..'\\'
	end
	Favorites_ADD(fname, fpath)
	Favorites_ListFILL()
	Favorites_SaveList()
end

function Favorites_AddCurrentBuffer()
	Favorites_ADD(props['FileNameExt'], props['FilePath'])
	Favorites_ListFILL()
	Favorites_SaveList()
end

function Favorites_DeleteItem()
	local idx = list_favorites:get_selected_item()
	if idx == -1 then return end
	list_favorites:delete_item(idx)
	table.remove (list_fav_table, idx+1)
	Favorites_SaveList()
end

local function Favorites_OpenFile()
	local idx = list_favorites:get_selected_item()
	if idx == -1 then return end
	local fname = list_favorites:get_item_data(idx)
	if fname:match('\\$') then
		gui.chdir(fname)
		current_path = fname
		FileMan_ListFILL()
	else
		OpenFile(fname)
	end
end

local function Favorites_ShowFilePath()
	local sel_item = list_favorites:get_selected_item()
	if sel_item == -1 then return end
	local expansion = list_favorites:get_item_data(sel_item)
	editor:CallTipCancel()
	editor:CallTipShow(-2, expansion)
end

list_favorites:on_select(function()
	Favorites_ShowFilePath()
end)

list_favorites:on_double_click(function()
	Favorites_OpenFile()
end)

list_favorites:on_key(function(key)
	if key == 13 then -- Enter
		Favorites_OpenFile()
	elseif key == 46 then -- Delete
		Favorites_DeleteItem()
	end
end)

----------------------------------------------------------
-- tab1:list_func   Functions/Procedures
----------------------------------------------------------
local table_functions = {}
-- 1 - function names
-- 2 - line number
-- 3 - function paramaters with parentheses
local _sort = 'order'
local _backjumppos -- store position if jumping

local Lang2lpeg = {}
do
	local P, V, Cg, Ct, Cc, S, R, C, Carg, Cf, Cb, Cp, Cmt = lpeg.P, lpeg.V, lpeg.Cg, lpeg.Ct, lpeg.Cc, lpeg.S, lpeg.R, lpeg.C, lpeg.Carg, lpeg.Cf, lpeg.Cb, lpeg.Cp, lpeg.Cmt

	--@todo: переписать с использованием lpeg.Cf
	local function AnyCase(str)
		local res = P'' --empty pattern to start with
		local ch, CH
		for i = 1, #str do
			ch = str:sub(i,i):lower()
			CH = ch:upper()
			res = res * S(CH..ch)
		end
		assert(res:match(str))
		return res
	end

	local PosToLine = function (pos) return editor:LineFromPosition(pos-1) end

--v------- common patterns -------v--
	-- basics
	local EOF = P(-1)
	local BOF = P(function(s,i) return (i==1) and 1 end)
	local NL = P"\n"+P"\r\n"+P"\r"-- + P"\f" -- pattern matching newline, platform-specific. \f = page break marker
	local AZ = R('AZ','az')+"_"
	local N = R'09'
	local ANY =  P(1)
	local ESCANY = P'\\'*ANY + ANY
	local SINGLESPACE = S'\n \t\r\f'
	local SPACE = SINGLESPACE^1

	-- simple tokens
	local IDENTIFIER = AZ * (AZ+N)^0 -- simple identifier, without separators

	local Str1 = P'"' * ( ESCANY - (S'"'+NL) )^0 * (P'"' + NL)--NL == error'unfinished string')
	local Str2 = P"'" * ( ESCANY - (S"'"+NL) )^0 * (P"'" + NL)--NL == error'unfinished string')
	local STRING = Str1 + Str2

	-- c-like-comments
	local line_comment = '//' * (ESCANY - NL)^0*NL
	local block_comment = '/*' * (ESCANY - P'*/')^0 * (P('*/') + EOF)
	local COMMENT = (line_comment + block_comment)^1

	local SC = SPACE + COMMENT
	local IGNORED = SPACE + COMMENT + STRING
	-- special captures
	local cp = Cp() -- pos capture, Carg(1) is the shift value, comes from start_code_pos
	local cl = cp/PosToLine -- line capture, uses editor:LineFromPosition
	local par = C(P"("*(1-P")")^0*P")") -- captures parameters in parentheses
--^------- common patterns -------^--

	do --v------- asm -------v--
		-- redefine common patterns
		local SPACE = S' \t'^1

		local IGNORED = (ESCANY - NL)^0 * NL -- just skip line by line

		-- define local patterns
		local p = P"proc"
		local F = P"FRAME"
		-- create flags:
		F = Cg(F*Cc(true),'F')
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local par = C((ESCANY - NL)^0)
		local def1 = I*SPACE*(p+F)
		local def2 = p*SPACE*I*P','^-1
		local def = (SPACE+P'')*Ct((def1+def2)*(SPACE*par)^-1)*NL
		-- resulting pattern, which does the work
		local patt = (def + IGNORED + 1)^0 * EOF

		Lang2lpeg.Assembler = lpeg.Ct(patt)
	end --do --^------- ASM -------^--

	do --v------- Lua -------v--
		-- redefine common patterns
		local IDENTIFIER = IDENTIFIER*(P'.'*IDENTIFIER)^0*(P':'*IDENTIFIER)^-1
		-- LONG BRACKETS
		local long_brackets = #(P'[' * P'='^0 * P'[') *
			function (subject, i1)
				local level = _G.assert( subject:match('^%[(=*)%[', i1) )
				local _, i2 = subject:find(']'..level..']', i1, true)  -- true = plain "find substring"
				return (i2 and (i2+1)) or #subject+1--error('unfinished long brackets')
				-- ^ if unfinished long brackets then capture till EOF (at #subject+1)
		end
		local LUALONGSTR = long_brackets

		local multi  = P'--' * long_brackets
		local single = P'--' * (1 - NL)^0 * NL
		local COMMENT = multi + single
		local SC = SPACE + COMMENT

		local IGNORED = SPACE + COMMENT + STRING + LUALONGSTR

		-- define local patterns
		local f = P"function"
		local l = P"local"
		-- create flags
		l = Cg(l*SC^1*Cc(true),'l')^-1
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local funcdef1 = l*f*SC^1*I*SC^0*par -- usual function declaration
		local funcdef2 = l*I*SC^0*"="*SC^0*f*SC^0*par -- declaration through assignment
		local def = Ct(funcdef1 + funcdef2)
		-- resulting pattern, which does the work
		local patt = (def + IGNORED^1 + IDENTIFIER + 1)^0 * (EOF) --+ error'invalid character')

		Lang2lpeg.Lua = lpeg.Ct(patt)
	end --do --^------- Lua -------^--

	do --v----- Pascal ------v--
		-- redefine common patterns
		local IDENTIFIER = IDENTIFIER*(P'.'*IDENTIFIER)^0
		local STRING = P"'" *( ANY - (P"'"+NL) )^0 *(P"'"+NL) --NL == error'unfinished string')
		--^ there's no problem with pascal strings with double single quotes in the middle, like this:
		--  'first''second'
		--  in the loop, STRING just matches the 'first'-part, and then the 'second'.

		local multi1  = P'(*' *(1-P'*)')^0 * (P'*)' + EOF)--unfinished long comment
		local multi2  = P'{' *(1-P'}')^0 * (P'}' + EOF)--unfinished long comment
		local single = P'//' * (1 - NL)^0 * NL
		local COMMENT = multi1 + multi2 + single

		local SC = SPACE + COMMENT
		local IGNORED = SPACE + COMMENT + STRING

		-- define local patterns
		local f = AnyCase"function"
		local p = AnyCase"procedure"
		local c = AnyCase"constructor"
		local d = AnyCase"destructor"
		local restype = AZ^1
		-- create flags:
		-- f = Cg(f*Cc(true),'f')
		restype = Cg(C(restype),'')
		p = Cg(p*Cc(true),'p')
		c = Cg(c*Cc(true),'c')
		d = Cg(d*Cc(true),'d')
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local procdef = Ct((p+c+d)*SC^1*I*SC^0*par^-1)
		local funcdef = Ct(f*SC^1*I*SC^0*par^-1*SC^0*P':'*SC^0*restype*SC^0*P';')
		-- resulting pattern, which does the work
		local patt = (procdef + funcdef + IGNORED^1 + IDENTIFIER + 1)^0 * EOF

		Lang2lpeg.Pascal = lpeg.Ct(patt)
	end --^----- Pascal ------^--

	do --v----- C++ ------v--
		-- define local patterns
		local keywords = P'if'+P'else'+P'switch'+P'case'+P'while'+P'for'
		local nokeyword = -(keywords)
		local type = P"static "^-1*P"const "^-1*P"enum "^-1*P'*'^-1*IDENTIFIER*P'*'^-1
		local funcbody = P"{"*(ESCANY-P"}")^0*P"}"
		-- redefine common patterns
		local IDENTIFIER = P'*'^-1*P'~'^-1*IDENTIFIER
		IDENTIFIER = IDENTIFIER*(P"::"*IDENTIFIER)^-1
		-- create flags:
		type = Cg(type,'')
		-- create additional captures
		local I = nokeyword*C(IDENTIFIER)*cl
		-- definitions to capture:
		local funcdef = nokeyword*Ct((type*SC^1)^-1*I*SC^0*par*SC^0*(#funcbody))
		local classconstr = nokeyword*Ct((type*SC^1)^-1*I*SC^0*par*SC^0*P':'*SC^0*IDENTIFIER*SC^0*(P"("*(1-P")")^0*P")")*SC^0*(#funcbody)) -- this matches smthing like PrefDialog::PrefDialog(QWidget *parent, blabla) : QDialog(parent)
		-- resulting pattern, which does the work
		local patt = (classconstr + funcdef + IGNORED^1 + IDENTIFIER + ANY)^0 * EOF

		Lang2lpeg['C++'] = lpeg.Ct(patt)
	end --^----- C++ ------^--

	do --v----- JS ------v--
		-- redefine common patterns
		local NL = NL + P"\f"
		local regexstr = P'/' * (ESCANY - (P'/' + NL))^0*(P'/' * S('igm')^0 + NL)
		local STRING = STRING + regexstr
		local IGNORED = SPACE + COMMENT + STRING
		-- define local patterns
		local f = P"function"
		local m = P"method"
		local funcbody = P"{"*(ESCANY-P"}")^0*P"}"
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local funcdef =  Ct((f+m)*SC^1*I*SC^0*par*SC^0*(#funcbody))
		local eventdef = Ct(P"on"*SC^1*P'"'*I*P'"'*SC^0*(#funcbody))
		-- resulting pattern, which does the work
		local patt = (funcdef + eventdef + IGNORED^1 + IDENTIFIER + 1)^0 * EOF

		Lang2lpeg.JScript = lpeg.Ct(patt)
	end --^----- JS ------^--

	do --v----- VB ------v--
		-- redefine common patterns
		local SPACE = (S(" \t")+P"_"*S(" \t")^0*(P"\r\n"))^1
		local SC = SPACE
		local NL = (P"\r\n")^1*SC^0
		local STRING = P'"' * (ANY - (P'"' + P"\r\n"))^0*P'"'
		local COMMENT = (P"'" + P"REM ") * (ANY - P"\r\n")^0
		local IGNORED = SPACE + COMMENT + STRING
		local I = C(IDENTIFIER)*cl
		-- define local patterns
		local f = AnyCase"function"
		local p = AnyCase"property"
			local let = AnyCase"let"
			local get = AnyCase"get"
			local set = AnyCase"set"
		local s = AnyCase"sub"
		--local con=Cmt(AnyCase"const",(function(s,i) if _show_more then return i else return nil end end))
		--local dim=Cmt(AnyCase"dim",(function(s,i) if _show_more then return i else return nil end end))

		--local scr=P("<script>")
		--local stt=P("<stringtable>")

		local restype = (P"As"+P"as")*SPACE*Cg(C(AZ^1),'')
		let = Cg(let*Cc(true),'pl')
		get = Cg(get*Cc(true),'pg')
		set = Cg(set*Cc(true),'ps')
		p = NL*p*SC^1*(let+get+set)
		s = NL*Cg(s*Cc(true),'S')
		f = NL*Cg(f*Cc(true),'F')
		--dim = NL*Cg(dim*Cc(true),"D")
		--con = NL*Cg(con*Cc(true),"C")

		local e = NL*AnyCase"end"*SC^1*(AnyCase"sub"+AnyCase"function"+AnyCase"property")
		local body = (IGNORED^1 + IDENTIFIER + 1 - f - s - p - e)^0*e

		-- definitions to capture:
		f = f*SC^1*I*SC^0*par
		p = p*SC^1*I*SC^0*par
		s = s*SC^1*I*SC^0*par
		--con = con*SC^1*I
		--dim = dim*SC^1*I
		local def = Ct((f + s + p)*(SPACE*restype)^-1)*body --+ Ct(dim+con)
		-- resulting pattern, which does the work

		local patt = (def + IGNORED^1 + IDENTIFIER + (1-NL)^1 + NL)^0 * EOF

		Lang2lpeg.VisualBasic = lpeg.Ct(patt)
	end --^----- VB ------^--

	do --v------- Python -------v--
		-- redefine common patterns
		local SPACE = S' \t'^1
		local IGNORED = (ESCANY - NL)^0 * NL -- just skip line by line
		-- define local patterns
		local c = P"class"
		local d = P"def"
		-- create flags:
		c = Cg(c*Cc(true),'class')
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local def = (c+d)*SPACE*I
		def = (SPACE+P'')*Ct(def*SPACE^-1*par)*SPACE^-1*P':'
		-- resulting pattern, which does the work
		local patt = (def + IGNORED + 1)^0 * EOF

		Lang2lpeg.Python = lpeg.Ct(patt)
	end --do --^------- Python -------^--

	do --v----- Nemerle ------v--
		local IGNORED = SC
		local keywords = P'if'+P'else'+P'unless'+P'finally'+P'while'+P'for'+P'foreach'+P'try'+P'catch'+P'match'+P'when'+P'throw'+P'do'
		local nokeyword = -(keywords)
		local mod = P'public'+P'private'+P'static'+P'virtual'+P'def'+P'new'
		local funcbody = P"{"*(ESCANY-P"}")^0*P"}"

		local I = nokeyword*C(IDENTIFIER)*cl

		local typ = IDENTIFIER*(P'.'*IDENTIFIER)^0
		-- распознаем tuples в типе возвращаемом методом/функцией
		local tuple = typ*(SPACE^0*P'*'*SPACE^0*typ)^1
		local tot = tuple+typ
		-- распознаем коллекции/словари в возвращаемом типе
		local ar = typ*SPACE^0*P'['*SPACE^0*((tot*SPACE^0*P','*SPACE^0)^0*(tot*SPACE^0))^0*P']'
		local arr = typ*SPACE^0*P'['*SPACE^0*(((ar+tot)*SPACE^0*P','*SPACE^0)^0*((ar+tot)*SPACE^0))^0*P']'
		local type = arr+tot
		-- распознаем контракт метода
		local req = (P'requires'+P'ensures')*SPACE*(AZ+SPACE+R'09'+S'.,?!=></[]-+()*&^%$#@')^1

		-- методы/функции
		local method = nokeyword*Ct((mod*SPACE)^0*I*SPACE^0*par)*(SPACE^0*P':'*SPACE^0*type)^0*SC^0*req^0*(#funcbody)
		-- декларации методов интерфейсов
		local ifmethod = nokeyword*Ct((P'new'*SPACE)^0*I*SPACE^0*par)*SPACE^0*P':'*SPACE^0*type*SPACE^0*P';'

		local patt = (method + ifmethod + IGNORED^1 + IDENTIFIER + ANY)^0 * EOF

		Lang2lpeg['Nemerle'] = lpeg.Ct(patt)
	end --^----- Nemerle ------^--

	do --v------- nnCron -------v--
		-- redefine common patterns
		local IDENTIFIER = (ANY - SPACE)^1
		local SPACE = S' \t'^1
		local IGNORED = (ESCANY - NL)^0 * NL -- just skip line by line
		-- define local patterns
		local d = P":"
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local def = d*SPACE*I
		def = Ct(def*(SPACE*par)^-1)*IGNORED
		-- resulting pattern, which does the work
		local patt = (def + IGNORED + 1)^0 * EOF

		Lang2lpeg.nnCron = lpeg.Ct(patt)
	end --do --^------- nnCron -------^--

	do --v------- CSS -------v--
		-- helper
		local function clear_spaces(s)
			return s:gsub('%s+',' ')
		end
		-- redefine common patterns
		local IDENTIFIER = (ANY - SPACE)^1
		local NL = P"\r\n"
		local SPACE = S' \t'^1
		local IGNORED = (ANY - NL)^0 * NL -- just skip line by line
		local par = C(P"{"*(1-P"}")^0*P"}")/clear_spaces -- captures parameters in parentheses
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		-- definitions to capture:
		local def = Ct(I*SPACE*par)--*IGNORED
		-- resulting pattern, which does the work
		local patt = (def + IGNORED + 1)^0 * EOF

		Lang2lpeg.CSS = lpeg.Ct(patt)
	end --do --^------- CSS -------^--

	do --v----- * ------v--
		-- redefine common patterns
		local NL = P"\r\n"+P"\n"+P"\f"
		local SC = S" \t\160" -- без понятия что за символ с кодом 160, но он встречается в SciTEGlobal.properties непосредственно после [Warnings] 10 раз.
		local COMMENT = P'#'*(ANY - NL)^0*NL
		-- define local patterns
		local somedef = S'fFsS'*S'uU'*S'bBnN'*AZ^0 --пытаемся поймать что-нибудь, похожее на определение функции...
		local section = P'['*(ANY-P']')^1*P']'
		-- create flags
		local somedef = Cg(somedef, '')
		-- create additional captures
		local I = C(IDENTIFIER)*cl
		section = C(section)*cl
		local tillNL = C((ANY-NL)^0)
		-- definitions to capture:
		local def1 = Ct(somedef*SC^1*I*SC^0*(par+tillNL))
		local def2 = (NL+BOF)*Ct(section*SC^0*tillNL)*NL

		-- resulting pattern, which does the work
		local patt = (def2 + def1 + COMMENT + IDENTIFIER + 1)^0 * EOF
		-- local patt = (def2 + def1 + IDENTIFIER + 1)^0 * EOF -- чуть медленнее

		Lang2lpeg['*'] = lpeg.Ct(patt)
	end --^----- * ------^--

	do --v------- autohotkey -------v--
		-- redefine
		local NL = P'\n'+P'\r\n'
		-- local NL = S'\r\n'
		local ESCANY = P'`'*ANY + ANY
		
		-- helper
		local I = (ESCANY-S'(){},=:;\r\n')^1
		local LINE = (ESCANY-NL)^0
		local block_comment = '/*' * (ESCANY - P'*/')^0 * (P('*/') + EOF)
		local line_comment  = P';'*LINE*(NL + EOF)
		local COMMENT = line_comment + block_comment
		local BALANCED = P{ "{" * ((1 - S"{}") + V(1))^0 * "}" } -- capture balanced {}
		-- definitions to capture:
		local label     = C( I*P':'*#(1-S'=:'))*cl*LINE
		local keystroke = C( I*P'::' )*cl*LINE
		local hotstring = C( P'::'*I*P'::'*LINE )*cl
		local directive = C( P'#'*I )*cl*LINE
		local func      = C( I )*cl*par*(COMMENT+NL)^0*BALANCED
		local def = Ct( keystroke + label + hotstring + directive + func )
		-- resulting pattern, which does the work
		local patt = (SPACE^0*def + NL + COMMENT + LINE*NL)^0 * LINE*(EOF) --+ error'invalid character')

		Lang2lpeg.autohotkey = lpeg.Ct(patt)
	end --do --^------- autohotkey -------^--

	do --v----- SQL ------v--
		-- redefine common patterns
		--идентификатор может включать точку
		local IDENTIFIER = AZ * (AZ+N+P".")^0
		local STRING = (P'"' * (ANY - P'"')^0*P'"') + (P"'" * (ANY - P"'")^0*P"'")
		local COMMENT = ((P"--" * (ANY - NL)^0*NL) + block_comment)^1
		local SC = SPACE

		local cr = AnyCase"create"*SC^1
		local pr = AnyCase"proc"*AnyCase"edure"^0
		local vi = AnyCase"view"
		local tb = AnyCase"table"
		local tr = AnyCase"trigger"
		local IGNORED = SPACE + COMMENT + STRING
		-- create flags
		tr = Cg(cr*tr*SC^1*Cc(true),'tr')
		tb = Cg(cr*tb*SC^1*Cc(true),'tb')
		vi = Cg(cr*vi*SC^1*Cc(true),'vi')
		pr = Cg(cr*pr*SC^1*Cc(true),'pr')

		local I = C(IDENTIFIER)*cl
		--параметры процедур и вью - всё от имени до as
		local parpv = C((1-AnyCase"as")^0)*AnyCase"as"
		--параметры таблиц содержат комментарии и параметры
		local partb = C((P"("*(COMMENT + (1-S"()")+par)^1*P")"))
		-- -- definitions to capture:
		pr = pr*I*SC^0*parpv
		vi = vi*I*SC^0*parpv
		tb = tb*I*SC^0*partb
		tr = tr*I*SC^1*AnyCase"on"*SC^1*I --"параметр" триггера - идентификатор после I
		local def = Ct(( pr + vi + tb + tr))

		-- resulting pattern, which does the work
		local patt = (def + IGNORED^1 + IDENTIFIER + 1)^0 * EOF

		Lang2lpeg.SQL = lpeg.Ct(patt)
	end --^----- SQL ------^--

end

local Lang2CodeStart = {
	['Pascal']='^IMPLEMENTATION$',
}

local Lexer2Lang = {
	['asm']='Assembler',
	['cpp']='C++',
	['js']='JScript',
	['vb']='VisualBasic',
	['vbscript']='VisualBasic',
	['css']='CSS',
	['pascal']='Pascal',
	['python']='Python',
	['sql']='SQL',
	['lua']='Lua',
	['nncrontab']='nnCron',
}

local Ext2Lang = {}
do -- Fill_Ext2Lang
	local patterns = {
		[props['file.patterns.asm']]='Assembler',
		[props['file.patterns.cpp']]='C++',
		[props['file.patterns.wsh']]='JScript',
		[props['file.patterns.vb']]='VisualBasic',
		[props['file.patterns.wscript']]='VisualBasic',
		['*.css']='CSS',
		['*.sql']='SQL',
		[props['file.patterns.pascal']]='Pascal',
		[props['file.patterns.py']]='Python',
		[props['file.patterns.lua']]='Lua',
		[props['file.patterns.nemerle']]='Nemerle',
		[props['file.patterns.nncron']]='nnCron',
		['*.ahk']='autohotkey',
	}
	for i,v in pairs(patterns) do
		for ext in (i..';'):gmatch("%*%.([^;]+);") do
			Ext2Lang[ext] = v
		end
	end
end -- Fill_Ext2Lang

local function Functions_GetNames()
	_DEBUG.timerstart('Functions_GetNames')
	table_functions = {}
	if editor.Length == 0 then return end

	local ext = props["FileExt"]:lower() -- a bit unsafe...
	local lang = Ext2Lang[ext]

	local start_code = Lang2CodeStart[lang]
	local lpegPattern = Lang2lpeg[lang]
	if not lpegPattern then
		lang = Lexer2Lang[props['Language']]
		start_code = Lang2CodeStart[lang]
		lpegPattern = Lang2lpeg[lang]
		if not lpegPattern then
			start_code = Lang2CodeStart['*']
			lpegPattern = Lang2lpeg['*']
		end
	end
	local textAll = editor:GetText()
	local start_code_pos = start_code and editor:findtext(start_code, SCFIND_REGEXP) or 0

	-- lpegPattern = nil
	table_functions = lpegPattern:match(textAll, start_code_pos+1) -- 2nd arg is the symbol index to start with
	_DEBUG.timerstop('Functions_GetNames','lpeg')
end

local function Functions_ListFILL()
	if tonumber(props['sidebar.show'])~=1 or tab_index~=1 then return end
	if _sort == 'order' then
		table.sort(table_functions, function(a, b) return a[2] < b[2] end)
	else
		table.sort(table_functions, function(a, b) return a[1]:lower() < b[1]:lower() end)
	end
	-- remove duplicates
	for i = #table_functions, 2, -1 do
		if table_functions[i][2] == table_functions[i-1][2] then
			table.remove (table_functions, i)
		end
	end
	list_func:clear()

	local function emptystr(...) return '' end
	local function GetParams (funcitem)
		return (funcitem[3] and ' '..funcitem[3]) or ''
	end
	local function GetFlags (funcitem)
		local res = ''
		local add = ''
		for flag,value in pairs(funcitem) do
			if type(flag)=='string' then
				if type(value)=='string' then	add = flag .. value
				elseif type(value)=='number' then add = flag..':'..value
				else add = flag end
				res = res .. '['.. add ..']'
			end
		end
		if res~='' then res = res .. ' ' end
		return res or ''
	end
	if not _show_params then GetParams = emptystr end
	if not _show_flags then GetFlags = emptystr end

	local function fixname (funcitem)
		return GetFlags(funcitem)..funcitem[1]..GetParams(funcitem)
	end
	for _, a in ipairs(table_functions) do
		local funcname = fixname(a)
		if tonumber(props["editor.unicode.mode"]) == IDM_ENCODING_DEFAULT then
			funcname = funcname:to_utf8(editor:codepage())
		end
		list_func:add_item(funcname, a[2])
	end
end

function Functions_SortByOrder()
	_sort = 'order'
	Functions_ListFILL()
end

function Functions_SortByName()
	_sort = 'name'
	Functions_ListFILL()
end

function Functions_ToggleParams ()
	_show_params = not _show_params
	Functions_ListFILL()
end

function Functions_ToggleFlags ()
	_show_flags = not _show_flags
	Functions_ListFILL()
end

local function Functions_GotoLine()
	local sel_item = list_func:get_selected_item()
	if sel_item == -1 then return end
	local pos = list_func:get_item_data(sel_item)
	if pos then
		ShowCompactedLine(pos)
		editor:GotoLine(pos)
		gui.pass_focus()
	end
end

list_func:on_double_click(function()
	Functions_GotoLine()
end)

list_func:on_key(function(key)
	if key == 13 then -- Enter
		Functions_GotoLine()
	end
end)

----------------------------------------------------------
-- tab1:list_bookmarks   Bookmarks
----------------------------------------------------------
local table_bookmarks = {}
local bm_file = props['SciteUserHome'].."\\bookmarks.lst"
local function GetBufferNumber()
	return tonumber(props['BufferNumber']) or 1
end

local function Bookmark_Add(line_number)
	local line_text = editor:GetLine(line_number)
	if line_text == nil then line_text = '' end
	line_text = line_text:gsub('^%s+', ''):gsub('%s+', ' ')
	if line_text == '' then
		line_text = ' - empty line - ('..(line_number+1)..')'
	end
	for _, a in ipairs(table_bookmarks) do
		if a.FilePath == props['FilePath'] and a.LineNumber == line_number then
		return end
	end
	local bmk = {}
	bmk.FilePath = props['FilePath']
	bmk.BufferNumber = GetBufferNumber()
	bmk.LineNumber = line_number
	if tonumber(props["editor.unicode.mode"]) == IDM_ENCODING_DEFAULT then
		line_text = line_text:to_utf8(editor:codepage())
	end
	bmk.LineText = line_text
	table_bookmarks[#table_bookmarks+1] = bmk
end

local function Bookmark_Delete(line_number)
	for i = #table_bookmarks, 1, -1 do
		if table_bookmarks[i].FilePath == props['FilePath'] then
			if line_number == nil then
				table.remove(table_bookmarks, i)
			elseif table_bookmarks[i].LineNumber == line_number then
				table.remove(table_bookmarks, i)
				break
			end
		end
	end
end

local function Bookmarks_ListFILL()
	if tonumber(props['sidebar.show'])~=1 or tab_index~=1 then return end
	table.sort(table_bookmarks, function(a, b)
									return a.BufferNumber < b.BufferNumber or
											a.BufferNumber == b.BufferNumber and
											a.LineNumber < b.LineNumber
								end)
	list_bookmarks:clear()
	for _, bmk in ipairs(table_bookmarks) do
		list_bookmarks:add_item({bmk.BufferNumber, bmk.LineText}, {bmk.FilePath, bmk.LineNumber})
	end
end

local function Bookmarks_RefreshTable()
	Bookmark_Delete()
	for i = 0, editor.LineCount do
		if editor:MarkerGet(i) == 2 then
			Bookmark_Add(i)
		end
	end
	Bookmarks_ListFILL()
end

local function Bookmarks_GotoLine()
	local sel_item = list_bookmarks:get_selected_item()
	if sel_item == -1 then return end
	local pos = list_bookmarks:get_item_data(sel_item)
	if pos then
		scite.Open(pos[1]) -- FilePath
		ShowCompactedLine(pos[2]) -- LineNumber
		editor:GotoLine(pos[2])
		gui.pass_focus()
	end
end

list_bookmarks:on_double_click( Bookmarks_GotoLine )

list_bookmarks:on_key(function(key)
	if key == 13 then -- Enter
		Bookmarks_GotoLine()
	end
end)

AddEventHandler("OnSendEditor", function(id_msg, wp, lp)
	if id_msg == SCI_MARKERADD then -- wp = line_number, lp = marker_type
		if lp == 1 then Bookmark_Add(wp) Bookmarks_ListFILL() end
	elseif id_msg == SCI_MARKERDELETE then -- wp = line_number, lp = marker_type
		if lp == 1 then Bookmark_Delete(wp) Bookmarks_ListFILL() end
	elseif id_msg == SCI_MARKERDELETEALL then -- wp = marker_type
		if wp == 1 then Bookmark_Delete() Bookmarks_ListFILL() end
	end
end)

AddEventHandler("OnTabMove", function(from, to)
--[[
123 from4 56 to7 89
123 to4 56 from7 89
]]
	 if from < to then
		for _, bmk in ipairs(table_bookmarks) do
			if bmk.BufferNumber > from and bmk.BufferNumber <= to then
				bmk.BufferNumber = bmk.BufferNumber - 1  
			elseif bmk.BufferNumber == from then
				bmk.BufferNumber = to
			end
		end
	else
		for _, bmk in ipairs(table_bookmarks) do
			if bmk.BufferNumber == from then
				bmk.BufferNumber = to
			elseif bmk.BufferNumber >= to and bmk.BufferNumber < from then
				bmk.BufferNumber = bmk.BufferNumber + 1  
			end
		end
	end
	Bookmarks_ListFILL()
end)

AddEventHandler("OnClose", function(file)
	local bn = tonumber(props['BufferNumber'])
	if not bn then return end
--[[	for i = #table_bookmarks, 1, -1 do
		if table_bookmarks[i].FilePath == file then
			table.remove(table_bookmarks, i)
		end
		if table_bookmarks[i].BufferNumber<bn then table_bookmarks[i].BufferNumber = table_bookmarks[i].BufferNumber-1 end
	end]]
	Bookmarks_ListFILL()
end)

----------------------------------------------------------
-- tab2:list_abbrev   Abbreviations
----------------------------------------------------------
local function Abbreviations_ListFILL()
	list_abbrev:clear()
	local abbrev_filename = props['AbbrevPath']
	local abbr_table = ReadAbbrevFile(abbrev_filename)
	if not abbr_table then return end
	for i,v in ipairs(abbr_table) do
		list_abbrev:add_item({v.abbr, v.exp:gsub('\t','\\t'):gsub('%%%%','%%')}, v.exp)
	end
	list_abbrev:autosize(0)
end

local Abbreviations_HideExpansion
if Abbreviations_USECALLTIPS then
	Abbreviations_HideExpansion = function ()
		editor:CallTipCancel()
	end
else
	Abbreviations_HideExpansion = function ()
		editor:AnnotationClearAll()
	end
end

local scite_InsertAbbreviation = scite_InsertAbbreviation or scite.InsertAbbreviation
local function Abbreviations_InsertExpansion()
	local sel_item = list_abbrev:get_selected_item()
	if sel_item == -1 then return end
	local expansion = list_abbrev:get_item_data(sel_item)
	scite_InsertAbbreviation(expansion)
	gui.pass_focus() --don't need to call Abbreviations_HideExpansion(): on_focus will do
end

local function Abbreviations_ShowExpansion()
	local sel_item = list_abbrev:get_selected_item()
	if sel_item == -1 then return end
	local expansion = list_abbrev:get_item_data(sel_item)
	expansion = expansion:gsub('\\\\','\4'):gsub('\\r','\r'):gsub('(\\n','\n'):gsub('\\t','\t'):gsub('\4','\\'):gsub('%%%%','%%')
	local cp = editor:codepage()
	if cp ~= 65001 then expansion = expansion:from_utf8(cp) end

	local cur_pos = editor.CurrentPos
	editor:GotoPos(cur_pos-1)
	editor:GotoPos(cur_pos)
	if Abbreviations_USECALLTIPS then
		editor:CallTipCancel()
		editor:CallTipShow(cur_pos, expansion)
	else
		editor:AnnotationClearAll()
		editor.AnnotationVisible = ANNOTATION_BOXED
		local linenr = editor:LineFromPosition(cur_pos)
		editor.AnnotationStyle[linenr] = 255 -- номер стиля, в котором вы задали параметры для аннотаций
		editor.AnnotationText[linenr] = expansion:gsub('\t', '    ')
	end
end

list_abbrev:on_double_click( Abbreviations_InsertExpansion )

list_abbrev:on_select( Abbreviations_ShowExpansion )

list_abbrev:on_key(function(key)
	if key == 13 then -- Enter
		Abbreviations_InsertExpansion()
	elseif key == 27 then -- ESC
		Abbreviations_HideExpansion()
	end
end)

--[[list_abbrev:on_focus(function(setfocus)
	if not setfocus then Abbreviations_HideExpansion() end
end)]]

----------------------------------------------------------
-- tab3:acsii   ASCII Table
----------------------------------------------------------
for i = 0, 255 do
	local symbol, html_number, html_code = gui.get_ascii(i)
	list_ascii:add_item({i, string.format("%02X",i), symbol:to_utf8(0), html_number, html_code})
end
list_ascii:autosize(0)
list_ascii:autosize(1)
list_ascii:autosize(2)
list_ascii:autosize(3)
list_ascii:autosize(4)

list_ascii:on_double_click( function (row, col, s)
	editor:ReplaceSel(s:from_utf8(editor.CodePage))
	gui.pass_focus()
end)

----------------------------------------------------------
-- Events
----------------------------------------------------------
local line_count

local function OnSwitch()
	_DEBUG.timerstart('OnSwitch')
	line_count = editor.LineCount
	if tab0:bounds() then -- visible FileMan
		local path = props['FileDir']
		if path == '' then return end
		current_path = path:gsub('\\$','')..'\\'
		FileMan_ListFILL()
	elseif tab1:bounds() then -- visible Funk/Bmk
		Functions_GetNames()
		Functions_ListFILL()
		Bookmarks_ListFILL()
	elseif tab2:bounds() then -- visible Abbrev
		Abbreviations_ListFILL()
	end
	_DEBUG.timerstop('OnSwitch')
	gui.pass_focus()
end
AddEventHandler("OnSwitchFile", OnSwitch)
AddEventHandler("OnOpen", OnSwitch)
AddEventHandler("OnSave", OnSwitch)

tabs:on_select(function(ind)
	props['sidebar.active.tab'] = ind
	tab_index = ind
	OnSwitch()
end)

--- Функции показывающие/прячущие боковую панель
function OnSwitchTab()
	local tab_idx = tonumber(props['sidebar.active.tab'])
	if tab_idx then
		tabs:select_tab(tab_idx)
	end
end
local SideBar_Show, SideBar_Hide
if win then
	SideBar_Show = function()
		win_parent:show()
		props['sidebar.show'] = 1
		OnSwitchTab()
	end
	SideBar_Hide = function()
		win_parent:hide()
		props['sidebar.show'] = 0
	end
else
	SideBar_Show = function()
		gui.set_panel(win_parent, sidebar_position)
		props['sidebar.show'] = 1
		OnSwitchTab()
	end
	SideBar_Hide = function()
		gui.set_panel()
		props['sidebar.show'] = 0
	end
end

--- Переключает отображение боковой панели
function SideBar_ShowHide()
	if tonumber(props['sidebar.show']) == 1 then
		SideBar_Hide()
	else
		SideBar_Show()
	end
end

-- Обновление списков Functions и Bookmarks при изменении кол-ва строк в активном документе
AddEventHandler("OnInit", function() -- событие OnUpdateUI доступно только после инициализации OnInit
AddEventHandler("OnUpdateUI", function()
	if (editor.Focus and line_count) then
		local line_count_new = editor.LineCount
		local def_line_count = line_count_new - line_count
		if def_line_count ~= 0 then
			if tab1:bounds() then -- visible Funk/Bmk
				local cur_line = editor:LineFromPosition(editor.CurrentPos)
				for i = 1, #table_functions do
					local table_line = table_functions[i][2]
					if table_line > cur_line then
						table_functions[i][2] = table_line + def_line_count
					end
				end
				Functions_ListFILL()
				Bookmarks_RefreshTable()
			end
			line_count = line_count_new
		end
	end
end)
end, 'RunOnce')
----------------------------------------------------------
-- Go to function definition
----------------------------------------------------------

-- По имени функции находим строку с ее объявлением (инфа берется из table_functions)
local function Func2Line(funcname)
	if not next(table_functions) then
		Functions_GetNames()
	end
	for i = 1, #table_functions do
		if funcname == table_functions[i][1] then
			return table_functions[i][2]
		end
	end
end

-- Переход на строку с объявлением функции
local function JumpToFuncDefinition()
	local funcname = GetCurrentWord()
	local line = Func2Line(funcname)
	if line then
		_backjumppos = editor.CurrentPos
		editor:GotoLine(line)
		return true -- обрываем дальнейшую обработку OnDoubleClick (выделение слова и пр.)
	end
end

local function JumpBack()
	if _backjumppos then
		editor:GotoPos(_backjumppos)
		_backjumppos = nil
	end
end

AddEventHandler("OnDoubleClick", function(shift, ctrl, alt)
-- AddEventHandler("OnClick", function(shift, ctrl, alt)
	if shift then
		return JumpToFuncDefinition()
	end
end)

AddEventHandler("OnKey", function(key, shift, ctrl, alt, char)
	if editor.Focus and ctrl then
		if key == 188 then -- '<'
			JumpBack()
		elseif key == 190 then -- '>'
			JumpToFuncDefinition()
		end
	end
end)

----------------------------------------------------------
-- Show Current Colour
----------------------------------------------------------
local function SetColour(colour)
	if colour:match('%x%x%x%x%x%x') then
		-- Set colour's value HEX
		memo_path:set_memo_colour("", "#"..colour)
	else
		-- Set default colour
		local def_bg = editor.StyleBack[32]
		local b = math.floor(def_bg / 65536)
		local g = math.floor((def_bg - b*65536) / 256)
		local r = def_bg - b*65536 - g*256
		local rgb_hex = string.format('#%2X%2X%2X', r, g, b)
		memo_path:set_memo_colour("", rgb_hex)
	end
end

AddEventHandler("OnDwellStart", function(pos, cur_word)
	if pos ~= 0 then
		SetColour(cur_word)
	end
end)

props["dwell.period"] = 50

local cur_word_old = ""
AddEventHandler("OnKey", function()
	if editor.Focus then
		local cur_word = GetCurrentWord() -- слово, на котором стояла каретка ДО ТОГО КАК ЕЁ ПЕРЕМЕСТИЛИ
		if cur_word ~= cur_word_old then
			SetColour(cur_word)
			cur_word_old = cur_word
		end
	end
end)

if not win then
	function MoveSideBarLeft()
		props['sidebar.position'] = 'left'
		gui.set_panel()
		gui.set_panel(win_parent, 'left')
		tabs:context_menu {
			'Панель справа|MoveSideBarRight'
		}
	end

	function MoveSideBarRight()
		props['sidebar.position'] = 'right' 
		gui.set_panel()
		gui.set_panel(win_parent, 'right')
		tabs:context_menu {
			'Панель слева|MoveSideBarLeft'
		}
	end
	
	props['sidebar.position'] = sidebar_position
	
	if sidebar_position == 'left' then
		tabs:context_menu {
			'Панель справа|MoveSideBarRight',
		}
	else
		tabs:context_menu {
			'Панель слева|MoveSideBarLeft',
		}
	end
end

list_dir:context_menu {
	'Выбрать папку...|FileMan_ChangeDir',
	'Показать все файлы|FileMan_MaskAllFiles',
	'C расширением выделенного файла|FileMan_MaskOnlyCurrentExt',
	'Показать в Проводнике|FileMan_Explore',
	'Перейти к текущему файлу|FileMan_SelectCurrentFile',
	'', -- separator
	'Открыть в SciTE|FileMan_OpenSelectedItems',
	'Выполнить|FileMan_FileExec',
	'Выполнить с параметрами|FileMan_FileExecWithParams',
	'', -- separator
	'Копировать в...|FileMan_FileCopy',
	'Переместить в...|FileMan_FileMove',
	'Переименовать|FileMan_FileRename',
	'Удалить файл\tDel|FileMan_FileDelete',
	'', -- separator
	'Добавить в Избранное\tIns|Favorites_AddFile',
}

list_favorites:context_menu {
	'Добавить текущий файл|Favorites_AddCurrentBuffer',
	'Удалить\tDel|Favorites_DeleteItem',
}

list_func:context_menu {
	'Сортировать по порядку|Functions_SortByOrder',
	'Сортировать по имени|Functions_SortByName',
	'Показать/скрыть флаги|Functions_ToggleFlags',
	'Показать/скрыть параметры|Functions_ToggleParams',
}

--[[function MemoCopy()
	editor:CopyText(memo_path:get_text())
end

memo_path:context_menu{
	'Копировать|MemoCopy',
}]]
--========================================================
-- now show SideBar:
if props['sidebar.restore'] == '1' then
	SideBar_Show()
elseif tonumber(props['sidebar.show'])==1 then
	AddEventHandler("OnInit", SideBar_Show, 'RunOnce')
end
