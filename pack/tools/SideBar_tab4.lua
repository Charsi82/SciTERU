-------------------------
-- Demo Tab for SideBar
-------------------------
return function(tabs, panel_width, colorback, colorfore)
	if props['sidebar.demo'] ~= '1' then return end
	local tab4 = gui.panel(panel_width)
	local label_text
	local label_icon
	local callbacks = {}
	local BM_FILE_IDX = 2
	local BM_LINE_IDX = 13
	-------------------
	-- test bookmarks
	local tree_style = 0 + 0x0001 + 0x0002 + 0x0004 --  + 0x4000
	+ 0x0800
	local bm_test = true
	if bm_test then
		local bm_file = props['SciteUserHome'] .. "\\bookmarks.lst"
		local ini = gui.ini_file(bm_file)

		--[[ treeview styles
#define TVS_HASBUTTONS          0x0001
#define TVS_HASLINES            0x0002
#define TVS_LINESATROOT         0x0004
#define TVS_EDITLABELS          0x0008
#define TVS_DISABLEDRAGDROP     0x0010
#define TVS_SHOWSELALWAYS       0x0020
#define TVS_RTLREADING          0x0040

#define TVS_NOTOOLTIPS          0x0080
#define TVS_CHECKBOXES          0x0100
#define TVS_TRACKSELECT         0x0200
#define TVS_SINGLEEXPAND        0x0400
#define TVS_INFOTIP             0x0800
#define TVS_FULLROWSELECT       0x1000
#define TVS_NOSCROLL            0x2000
#define TVS_NONEVENHEIGHT       0x4000
#define TVS_NOHSCROLL           0x8000  // TVS_NOSCROLL overrides this
]]

		local bm_label = tab4:add_label(0, "Bookmarks Tree:")
		bm_label:position(50, 70)
		local tree_bookmarks = tab4:add_tree(tree_style)
		local lw, lh = bm_label:size()
		tree_bookmarks:position(20, 70 + lh)
		tree_bookmarks:size(260, 300)
		tree_bookmarks:on_tip(function(item)
			local name = tree_bookmarks:tree_get_item_text(item)
			local line = tonumber(name:match("^%[(%d+)%]"))
			if not line then return tree_bookmarks:tree_get_item_data(item) end
		end)

		local function ShowCompactedLine(line_num)
			local function GetFoldLine(ln)
				while editor.FoldExpanded[ln] do ln = ln - 1 end
				return ln
			end
			while not editor.LineVisible[line_num] do
				local x = GetFoldLine(line_num)
				editor:ToggleFold(x)
				line_num = x - 1
			end
		end

		local function Bookmarks_GotoLine(sel_item)
			local name = tree_bookmarks:tree_get_item_text(sel_item)
			local line = tonumber(name:match("^%[(%d+)%]"))
			if line then
				local parent = tree_bookmarks:tree_get_item_parent(sel_item)
				local path = tree_bookmarks:tree_get_item_data(parent)
				-- editor:GotoLine(line-1)
				gui.pass_focus()
				if props['FilePath'] == path then
					ShowCompactedLine(line - 1)
					editor:GotoLine(line - 1)
				else
					scite.Open(path)
					event("OnUpdateUI"):register(function(e)
						e:removeThisCallback()
						ShowCompactedLine(line - 1)
						editor:GotoLine(line - 1)
						-- gui.pass_focus()
					end)
				end
			end
		end

		tree_bookmarks:on_key(function(key)
			if key == 13 then -- Enter
				local item = tree_bookmarks:tree_get_item_selected()
				Bookmarks_GotoLine(item)
			end
		end)

		local table_bookmarks = {}
		tree_bookmarks:on_double_click(Bookmarks_GotoLine)
		tree_bookmarks:set_iconlib()

		local function GetLineText(line_number)
			local ELLIPSIS_LEN = 30
			local line_text = editor:GetLine(line_number) or ""
			line_text = line_text:gsub('^%s+', ''):gsub('%s+', ' ')
			if line_text == '' then line_text = ' - empty line' end
			line_text = "[" .. (line_number + 1) .. "] " .. line_text
			if #line_text > ELLIPSIS_LEN then line_text = line_text:sub(1, ELLIPSIS_LEN - 3) .. "..." end
			return line_text
		end

		local function get_or_add_parent_item()
			local res = tree_bookmarks:tree_get_item(props['FileNameExt'])
			if not res then
				tree_bookmarks:tree_set_insert_mode('sort')
				res = tree_bookmarks:add_item(props['FileNameExt'], nil, BM_FILE_IDX, BM_FILE_IDX, props['FilePath'])
				tree_bookmarks:tree_set_insert_mode('last')
			end
			return res
		end

		local function Bookmark_BeforeAdd(line)
			ini:set_section(PathCollapse(props['FilePath']))
			ini:write_string(line, GetLineText(line))
			local parent_item = get_or_add_parent_item()
			tree_bookmarks:tree_remove_childs(parent_item)
			for i = 0, editor.LineCount do if (editor:MarkerGet(i) // 2 % 2 == 1) or (i == line) then tree_bookmarks:add_item(GetLineText(i), parent_item, BM_LINE_IDX) end end
			tree_bookmarks:tree_expand(parent_item)
		end

		local function Bookmark_BeforeDelete(line)
			local parent_item = get_or_add_parent_item()
			if not line then
				ini:remove_section(PathCollapse(props['FilePath']))
				tree_bookmarks:tree_remove_item(parent_item)
			else
				local bm_count = 0
				local ml = 0
				while true do
					ml = editor:MarkerNext(ml, 2)
					if (ml == -1) then break end
					bm_count = bm_count + 1
					ml = ml + 1
				end
				if bm_count < 2 then
					Bookmark_BeforeDelete()
				else
					ini:remove_key(line)
					local item_to_remove = tree_bookmarks:tree_get_item(GetLineText(line), parent_item)
					if item_to_remove then
						tree_bookmarks:tree_remove_item(item_to_remove)
						-- else
						-- print('item_to_remove not found',GetLineText(line))
					end
				end
			end
		end

		AddEventHandler("OnSendEditor", function(id_msg, wp, lp)
			if id_msg == SCI_MARKERADD then -- wp = line_number, lp = marker_type
				if lp == 1 then Bookmark_BeforeAdd(wp) end
			elseif id_msg == SCI_MARKERDELETE then -- wp = line_number, lp = marker_type
				if lp == 1 then Bookmark_BeforeDelete(wp) end
			elseif id_msg == SCI_MARKERDELETEALL then -- wp = marker_type
				if wp == 1 then Bookmark_BeforeDelete() end
			end
		end)

		--[[local BTN_TEST_ID = 0
local btn = gui.button("test",BTN_TEST_ID)
tab4:add(btn,"none")
btn:position(20,380)
btn:size(50,30)
callbacks[BTN_TEST_ID] = function() gui.pass_focus()
end]]

		local line_count = 0
		function LineCountFix()
			if line_count ~= editor.LineCount then
				line_count = editor.LineCount
				-- output:ClearAll()
				local parent_item
				local ml = 0
				while true do
					ml = editor:MarkerNext(ml, 2)
					if (ml == -1) then break end
					-- 				print(i, string.format("%X",editor:MarkerGet(i)))
					if not parent_item then
						parent_item = get_or_add_parent_item()
						tree_bookmarks:tree_remove_childs(parent_item)
					end
					tree_bookmarks:add_item(GetLineText(ml), parent_item, BM_LINE_IDX)
					ml = ml + 1
				end
			end
			return false
		end

		AddEventHandler("OnInit", function()
			local t = ini:get_sections()
			for _, sect in ipairs(t) do
				local full_path = PathExpand(sect)
				if shell.fileexists(full_path) then
					local parent_item = tree_bookmarks:add_item(sect:match('[^\\]+$'), nil, BM_FILE_IDX, BM_FILE_IDX, full_path)
					ini:set_section(sect)
					local keys = ini:get_keys()
					local lines = {}
					for _, key in ipairs(keys) do
						local caption = ini:read_string(key, "_line_")
						lines[#lines + 1] = {caption = caption, id = tonumber(caption:match("%[(%d+)%]")) or 0}
					end
					table.sort(lines, function(a, b) return a.id < b.id end)
					for _, v in ipairs(lines) do tree_bookmarks:add_item(v.caption, parent_item, BM_LINE_IDX) end
				else
					ini:remove_section(sect)
				end

			end
		end, true)

		AddEventHandler("OnUpdateUI", function()
			if editor.Focus and props['FileName'] ~= '' and tab4:bounds() then
				return LineCountFix() -- true - break event
			end
		end)

	end -- do

	local tree = tab4:add_tree(tree_style)
	do
		if colorback then tree:set_tree_colour(colorfore, colorback) end
		-- tab4:add(tree, "none")
		tree:position(550, 5)
		tree:size(170, 370)
		-- local itm1 = tree:add_item("item1", nil, 1) -- ("caption" [, parent_item = root][, icon_idx=-1][, sel_icon_idx=-1][, data=nil])
		--[[tree:add_item("item2", itm1, 1)
tree:add_item("item3")
local itm4 = tree:add_item("item4")
local itm5 = tree:add_item("item5", itm4,-1)
local itm6 = tree:add_item("item6", itm5,0)
local itm7 = tree:add_item("item7", itm6,0)]]
		-- local imgcnt = tree:set_iconlib([[toolbar\cool.dll]]) --path [, small_size = true] return count of loaded icons
		-- local imgcnt = tree:set_iconlib([[%windir%\system32\shell32.dll]], false ) 
		-- local imgcnt = tree:set_iconlib([[%SystemRoot%\system32\shell32.dll]])
		--[[tree:add_item("qwerty", nil, 1)
tree:add_item("qwerty2", nil, 1)
tree:add_item("qwerty3", nil, 1)]]

		local imgcnt = tree:set_iconlib([[toolbar\cool.dll]]) -- path [, small_size = true] return count of loaded icons
		-- local imgcnt = tree:set_iconlib([[%windir%\system32\shell32.dll]] )
		local lib1_itm = tree:add_item("cool icons", nil, 16)
		for icon_idx = 0, imgcnt - 1 do tree:add_item("item_" .. icon_idx, lib1_itm, icon_idx, icon_idx, icon_idx) end
		tree:tree_set_insert_mode('first')
		local lib2_itm = tree:add_item("sorted", lib1_itm, 16)
		tree:tree_set_insert_mode('sort')
		for icon_idx = 0, 10 do tree:add_item("item_" .. icon_idx, lib2_itm, icon_idx, icon_idx, icon_idx) end
		tree:tree_set_insert_mode(lib2_itm)
		local lib3_itm = tree:add_item("reversed", lib1_itm, 16)
		tree:tree_set_insert_mode('first')
		for icon_idx = 0, 10 do tree:add_item("item_" .. icon_idx, lib3_itm, icon_idx, icon_idx, icon_idx) end
		-- tree:set_selected_item(itm5)
		-- print("test:",tree:tree_item_get_text(itm5))
		tree:on_select(function(sel_item) label_text:set_text('select tree:' .. tree:tree_get_item_text(sel_item)) end)

		tree:on_double_click(function(sel_item)
			local data = tree:tree_get_item_data(sel_item)
			label_text:set_text('db click tree:' .. tostring(data))
			label_icon:set_icon([[toolbar\cool.dll]], data)
		end)
		tab4:set_tiptext(tree:get_ctrl_id(), 'tips tree')
		-- tree:set_tree_editable(true)
		function tree_test_menu() label_text:set_text('menu for tree ' .. (tree:get_ctrl_id() or '[none]') .. ' clicked') end -- ok

		tree:context_menu{"test|tree_test_menu"} -- ok
	end
	-------------------
	-- listbox
	-- local listbox1 = nil
	local ListBoxID = 77
	local listbox = tab4:add_listbox(ListBoxID)
	do
		listbox:position(730, 5)
		listbox:size(75, 145)
		for i = 1, 15 do listbox:append("line" .. i, i * i) end
		callbacks[ListBoxID] = function(state)
			--[[
	1 - clicked
	2 - db clicked
	4 - focused
	5 - unfocused
	]]
			if state == 2 then
				-- 	print("db clicked list:",listbox:get_line_text(listbox:select()))
				label_text:set_text("list db clicked:" .. listbox:get_line_text(listbox:select()))
			end
			if state == 1 then label_text:set_text("list clicked:" .. listbox:get_line_data(listbox:select())) end
		end
		-- listbox:insert(idx, "caption"[,data]) -- ok
		-- listbox:select(10) -- ok
		listbox:remove(2) -- ok
		-- print("lb1:",listbox:count())
		-- listbox:clear() --ok
		-- print("lb2:",listbox:count()) -- ok
		-- print("select=",listbox:select()) --ok
		-- print("line_0_text=",listbox:get_line_text(4))
		-- print("line_0_data=",listbox:get_line_data(4))
		function list_test_menu() label_text:set_text('menu for list clicked') end
		listbox:context_menu{"test|list_test_menu"}
	end
	-------------------
	local trbar1
	local Button1_ID = 11
	local btn = tab4:add_button(Button1_ID, "btn1")
	btn:position(290, 260)
	btn:size(40, 30)
	callbacks[Button1_ID] = function(state)
		label_text:set_text('btn_1_clicked')
		trbar1:select(30, 60)
	end

	local Button2_ID = 12
	local btn2 = tab4:add_button(Button2_ID, "btn2")
	-- tab4:add(btn2, "none")
	btn2:position(290 + 5 + btn:size(), 260)
	btn2:size(40, 30)
	tab4:set_tiptext(Button2_ID, "кнопка с текстом")
	callbacks[Button2_ID] = function(state)
		label_text:set_text('btn_2_clicked')
		trbar1:sel_clear()
	end

	local Button9_ID = 99
	local btn9 = tab4:add_button(Button9_ID, "btn9")
	-- tab4:add(btn9, "none")
	btn9:position(btn2:position() + 5 + btn2:size(), 260)
	btn9:size(40, 30)
	btn9:set_icon([[toolbar\cool.dll]], 59)
	tab4:set_tiptext(Button9_ID, "кнопка с иконкой", '', true, true)

	local Checkbox1_ID = 33
	local chbox = tab4:add_checkbox("checkbox", Checkbox1_ID, false) -- caption, id, treestate

	chbox:position(btn9:position() + 25 + btn9:size(), 260)
	callbacks[Checkbox1_ID] = function(state) label_text:set_text('checkbox state:' .. (chbox:check())) end
	chbox:check(1)

	local Checkbox2_ID = 34
	local chbox3 = tab4:add_checkbox("threestate", Checkbox2_ID, true) -- caption, id, treestate
	chbox3:position(chbox:position(), 275)
	callbacks[Checkbox2_ID] = function(state) label_text:set_text('check3box:' .. chbox3:check()) end
	tab4:set_tiptext(Checkbox2_ID, "checkbox 3-state")

	callbacks[Button9_ID] = function(state)
		local sel_item = tree:tree_get_item_selected()
		if sel_item then tree:tree_remove_item(sel_item) end
	end

	local RadioBtn1_ID = 44
	local rbtn = tab4:add_radiobutton("radio_1", RadioBtn1_ID, true) -- caption, id, auto
	rbtn:position(290, 295)
	rbtn:check(1)
	callbacks[RadioBtn1_ID] = function(state) label_text:set_text('radio_1_clicked') end

	local RadioBtn2_ID = 55
	local rbtn2 = tab4:add_radiobutton("radio_2", RadioBtn2_ID, true)
	rbtn2:position(290 + 5 + rbtn:size(), 295)
	callbacks[RadioBtn2_ID] = function(state) label_text:set_text('radio_2_clicked') end

	-- LABEL --
	--[[
-- text alignment
left   0
center 1
right  2

-- markers
#define SS_BLACKRECT        0x00000004
#define SS_GRAYRECT         0x00000005
#define SS_WHITERECT        0x00000006

-- icon, bitmap
#define SS_ICON             0x00000003L
#define SS_BITMAP           0x0000000EL

-- borders
#define SS_BLACKFRAME       0x00000007
#define SS_GRAYFRAME        0x00000008
#define SS_WHITEFRAME       0x00000009
#define SS_ETCHEDFRAME      0x00000012

-- text with border
#define SS_SUNKEN           0x00001000 
]]

	------- label icon -------
	label_icon = tab4:add_label(0x00000003)
	label_icon:position(290, 155)
	label_icon:set_icon([[toolbar\cool.dll]], 25)
	--------------------------
	label_text = tab4:add_label(1 --  + 0x00001000
	+ 0x00001000 --  + 0x00000003
	-- + 0x0000000E
	, "StaticText")
	label_text:position(290 + 5 + label_icon:size(), 155)
	--------------------------
	--[[ trackbar style option
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
	local TrackBarID = 777
	trbar1 = tab4:add_trackbar(0x20 + 0x200, TrackBarID) -- style, id
	trbar1:position(290, 235)
	trbar1:size(250, 20)
	-- trbar1:size(20, 250) -- vert
	-- trbar1:range(20,80)
	--[[
-- style TBS_ENABLESELRANGE required
local selmin, selmax = trbar1:select() -- get selmin, selmax
trbar1:select(20,80) -- set selection
trbar1:sel_clear() -- clear selection
]]
	local prog
	tab4:on_scroll(function(id, pos)
		if id == TrackBarID then
			label_text:set_text("scroll to: " .. pos)
			prog:set_progress_pos(pos)
		end
	end)

	--[[
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L
#define CBS_UPPERCASE         0x2000L
#define CBS_LOWERCASE         0x4000L
]]
	-----------------------------------
	local cbbox = tab4:add_combobox(90, 0 -- id, style
	+ 0x0001 + 0x0040)
	cbbox:position(290, 5)
	cbbox:size(250, 85)
	for i = 1, 10 do cbbox:cb_append("text1_" .. i) end
	cbbox:cb_setcursel(1)
	-----------------------------------
	local cbbox = tab4:add_combobox(89, 0 -- id, style
	+ 0x0002 + 0x0040)
	cbbox:position(290, 90)
	cbbox:size(250, 20)
	for i = 1, 10 do cbbox:cb_append("text2_" .. i) end
	cbbox:cb_setcursel(2)
	------------------------------------
	local ComboBox_ID = 88
	local cbbox = tab4:add_combobox(ComboBox_ID, 0 -- id, style
	+ 0x0003 + 0x0040)
	cbbox:position(290, 120)
	cbbox:size(250, 10)
	for i = 1, 10 do cbbox:cb_append("text3_" .. i) end

	-- cbbox:cb_items_h(15) -- ok
	cbbox:cb_setcursel(1) -- ok
	-- print(cbbox:cb_getcursel(), cbbox:get_text())
	cbbox:cb_setcursel(-1) -- clear selection
	-- cbbox:cb_clear()
	cbbox:cb_setcursel(3)
	callbacks[ComboBox_ID] = function(state)
		if state == 1 then
			-- 			print(cbbox:get_text(), state)
			label_text:set_text("cbox_text:" .. cbbox:get_text())
		end
	end

	local Button3_ID = 133
	local btn3 = tab4:add_button(Button3_ID, "Tree 'Explorer'")
	btn3:position(290, 200)
	btn3:size(80, 20)
	tab4:set_tiptext(Button3_ID, "Задает тему для дерева")

	local tree_explorer = false
	callbacks[Button3_ID] = function(state)
		tree_explorer = not tree_explorer
		tree:set_theme(tree_explorer)
		btn3:set_text(tree_explorer and "Tree 'Normal'" or "Tree 'Explorer'")
	end

	local Button4_ID = 134
	local btn4 = tab4:add_button(Button4_ID, "Tree Edit ON")
	btn4:position(btn3:position() + 5 + btn3:size(), 200)
	btn4:size(80, 20)
	tab4:set_tiptext(Button4_ID, "Вкл\\откл. редактирование дерева")

	local tree_editable = false
	callbacks[Button4_ID] = function(state)
		tree_editable = not tree_editable
		tree:set_tree_editable(tree_editable)
		btn4:set_text(tree_editable and "Tree Edit OFF" or "Tree Edit ON")
	end

	--[[
#define ES_LEFT             0x0000L
#define ES_CENTER           0x0001L
#define ES_RIGHT            0x0002L
#define ES_MULTILINE        0x0004L
#define ES_UPPERCASE        0x0008L
#define ES_LOWERCASE        0x0010L
#define ES_PASSWORD         0x0020L
#define ES_AUTOVSCROLL      0x0040L
#define ES_AUTOHSCROLL      0x0080L
#define ES_NOHIDESEL        0x0100L
#define ES_OEMCONVERT       0x0400L
#define ES_READONLY         0x0800L
#define ES_WANTRETURN       0x1000L
#define ES_NUMBER           0x2000L
]]
	local editbox = tab4:add_editbox("7", 0 -- + 0x2000
	+ 0x0001 -- + 0x0002
	+ 0x0100 -- + 0x0800
	-- + 0x0004
	-- + 0x1000
	, 555)
	editbox:position(480, 155)
	editbox:size(60, 20)

	-------------- progress bar ---------------
	prog = tab4:add_progress() -- args: vertical=false, hasborder=false, smooth=false, smoothrevers=false
	prog:position(290, 182)
	prog:size(250, 10)
	prog:set_progress_pos(40)
	-- prog:progress_set_barcolor( "#FF0000" ) -- don't work
	-- prog:progress_set_bkcolor( "#00FF00" )-- don't work

	-- btn for test 'go' method
	local Button5_ID = 135
	local btn5 = tab4:add_button(Button5_ID, "Progress GO")
	btn5:position(btn4:position() + 5 + btn4:size(), 200)
	btn5:size(70, 20)
	callbacks[Button5_ID] = function() prog:progress_go() end

	--------------------------------------------

	tab4:context_menu{"Скрыть панель|" .. (IDM_TOOLS + tonumber(props['CN_SIDEBAR']))} -- run IDM_TOOLS+140

	----- iconed_list -------
	local iconed_list = tab4:add_list(false, false, false) -- singlesel, multiline, large_icons
	iconed_list:set_iconlib()
	iconed_list:position(290, 315)
	iconed_list:size(220, 55)
	for i = 1, 10 do iconed_list:add_item("iconed_list_item" .. i, nil, i * 2) end

	--------- updown ---------
	--[[
#define UDS_WRAP                0x0001
#define UDS_ALIGNRIGHT          0x0004
#define UDS_ALIGNLEFT           0x0008
#define UDS_AUTOBUDDY           0x0010
#define UDS_ARROWKEYS           0x0020
#define UDS_HORZ                0x0040
#define UDS_NOTHOUSANDS         0x0080
#define UDS_HOTTRACK            0x0100
]]
	local style = 0 + 0x0004 + 0x0020 + 0x0080
	-- +0x0100
	-- +0x0001

	local updown = tab4:add_updown(editbox, style) -- buddy, style
	updown:set_range(-50, 50)
	updown:set_current(7)

	-------- groupbox ---------
	local grbox = tab4:add_groupbox('Дата и время')
	grbox:position(20, 10)
	grbox:size(150, 50)

	-------- label with timer --------

	label_timer = tab4:add_label(0x00001000 + 1, os.date("%x %H:%M:%S"))
	-- grbox:center_h(label_timer)
	label_timer:position(26, 31)
	-- print(label_timer:size())
	tab4:on_timer(function() if label_timer then label_timer:set_text(os.date("%x %H:%M:%S")) end end)

	-------- positioner ---------

	local Button6_ID = 136
	local btn3 = tab4:add_button(Button6_ID, "-><-")
	btn3:position(20, 395)
	btn3:size(45, 20)

	local move_modes = {"off", "on"}
	local move_mode = 0
	local tr_state = 0
	callbacks[Button6_ID] = function()
		move_mode = (move_mode + 1) % (#move_modes)
		label_text:set_text('mode:' .. move_modes[move_mode + 1])
	end

	-- ctrl to need move
	moved_ctrl = listbox

	label_text:set_text("mode[" .. move_modes[move_mode + 1] .. "]")
	local step = 1
	AddEventHandler("OnKey", function(key, shift, ctrl, alt, char)
		if tab4:bounds() then
			if not moved_ctrl then
				print('no ctrl to move')
				return true
			end
			-- if key == 32 then move_mode = (move_mode+1)%(#move_modes) print('mode:',move_modes[move_mode]) end
			if move_mode == 0 then return false end
			-- print('tab4 onkey', key)
			if move_mode == 1 and key > 36 and key < 41 then
				local _, _, _, w, h = moved_ctrl:bounds()
				local x, y = moved_ctrl:position()
				if ctrl then -- position
					if key == 38 then y = y - step end
					if key == 40 then y = y + step end
					if y < 0 then y = 0 end
					if key == 37 then x = x - step end
					if key == 39 then x = x + step end
					if x < 0 then x = 0 end
				elseif alt then -- size
					if key == 38 then h = h - step end
					if key == 40 then h = h + step end
					if h < 5 then h = 5 end
					if key == 37 then w = w - step end
					if key == 39 then w = w + step end
				elseif shift then -- step
					if key == 38 then step = step - 5 end
					if key == 40 then step = step + 5 end
					if key == 37 then step = step - 1 end
					if key == 39 then step = step + 1 end
					if step < 1 then step = 1 end
					if step > 100 then step = 100 end
				end
				moved_ctrl:position(x, y)
				moved_ctrl:size(w, h)
				label_text:set_text(string.format("[ %d : %d ] [ %d : %d ][step = %d]", x, y, w, h, step))
			end
			return true -- true - break event
		end
	end)

	tab4:on_command(function(id, state)
		local cb_func = callbacks[id]
		if cb_func then
			cb_func(state)
			return true
		end
		return false
	end)
	---------------------------------  
	local Button99_ID = 99
	local btn99 = tab4:add_button(Button99_ID, "tip test")
	btn99:position(20, 420)
	btn99:size(100, 25)
	-- tab4:set_tiptext(Button99_ID , 'tips', "caption", false, true, 2)
	local function gen_color()
		local r, g, b = math.random(256) - 1, math.random(256) - 1, math.random(256) - 1
		return string.format("#%02X%02X%02X", r, g, b), string.format("#%02X%02X%02X", (128 + r) % 256 + 1, (128 + g) % 256 + 1, (128 + b) % 256 + 1)
	end
	local tt = tab4:add_tooltip(Button99_ID, true):set_text('text tips 1111')
	callbacks[Button99_ID] = function(state)
		local ct, cb = gen_color()
		local txt = ("text tips '%s' '%s'"):format(ct, cb)
		tt:set_color(gen_color()) -- text color, back color
		local icon = math.random(7) - 1
		tt:set_text(txt):set_caption('caption' .. icon, icon)
		-- print('set tips: ', txt)
	end

	local Button999_ID = 199
	local btn999 = tab4:add_button(Button999_ID, "restart")
	btn999:position(20, 455)
	btn999:size(100, 25)
	callbacks[Button999_ID] = function(state)
		scite.MenuCommand(IDM_SAVE)
		scite.ReloadStartupScript()
	end
	---------------------------------
	local cp = tab4:add_custom()
	cp:on_paint(function(a)
		-- print('paint')
		-- print(a.rectangle, a.draw_text)
		--[[
        #define PS_SOLID            0
#define PS_DASH             1       /* -------  */
#define PS_DOT              2       /* .......  */
#define PS_DASHDOT          3       /* _._._._  */
#define PS_DASHDOTDOT       4       /* _.._.._  */
#define PS_NULL             5
#define PS_INSIDEFRAME      6
#define PS_USERSTYLE        7
#define PS_ALTERNATE        8
#define PS_STYLE_MASK       0x0000000F
]]
		a:solid_brush("#6AFF55")
		a:set_pen("#FF0022", 1, 2) -- color, opt_width[0], opt_style[PS_SOLID]
		a:rectangle(20, 490, 120, 515)

		-- draw lines
		-- a:xor_pen(true)
		a:set_pen("#008000", 0)
		a:move_to(125, 492)
		a:line_to(240, 492)
		-- a:xor_pen(false)

		a:set_pen("#008000", 0, 1) -- DASH
		a:draw_line(125, 497, 240, 497) -- same as  a:move_to(125,497) a:line_to(240,497)

		a:set_pen("#008000", 0, 2) -- DOT
		a:draw_line(125, 502, 240, 502)

		a:set_pen("#008000", 0, 3) -- PS_DASHDOT
		a:draw_line(125, 507, 240, 507)

		a:set_pen("#008000", 0, 4) -- PS_DASHDOTDOT
		a:draw_line(125, 512, 240, 512)

		--[[
        #define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
]]
		local function points(pos_y)
		-- x0, y0, x1, y1, ...
		return 120, pos_y+410, 185, pos_y+480, 230, pos_y+430, 280, pos_y+450
		end
		a:select_stock(7)
		a:polyline(points(0))

		a:reset_pen()
		a:polybezier(points(0))

		--[[
#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24
]]
		-- a:text_align(0)
		a:color_back("#FFFFFF")
		a:color_text("#0055FF")
		a:draw_text('SciTE', 50, 494)

		a:solid_brush("#7365EF")
		a:select_stock(7) -- black pen
		a:rectangle(20, 525, 120, 550)
		
		a:solid_brush("#EFEC65")
		a:ellipse(130, 525, 230, 550)
		
		a:solid_brush("#CF6BE9")
		a:round_rect(130, 555, 230, 590, 30, 20)
		
		a:solid_brush("#6AFF55")
		a:chord(
		130, 555, 230, 590,
		130, 580,
		230, 580
		)
		
		a:polygone(points(200))
		
		--[[
		#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */
]]
		a:hatch_brush("#E90A0A", 5)
		a:rectangle(20, 555, 120, 590)
		--[[        math.randomseed(os.time())
        local tw, th = tab4:size()
        local cnt = tw*th/100
        if cnt > 1000 then cnt = 1000 end
        local math_random = math.random
		for i=1,cnt do
			local x = math_random(tw)
			local y = math_random(th)
			a:set_pixel(x,y)
		end]]
-- TODO перерисовать сайдбар при переключении в плавающее окно
	end)
	---------------------------------
	tabs:add_tab("Test", tab4, 37)
	return tab4
end
