-------------------------
-- Demo Tab for SideBar
-------------------------

return function(tabs, panel_width, colorback, colorfore)

	local tab4 = gui.panel(panel_width)
	local callbacks = {}

	-------- label with timer --------
	local label_timer = tab4:add_label(0x00001000 + 0x1)
	label_timer:position(16, 31)
	label_timer:set_text(os.date("%x %H:%M:%S"))
	
	----------- groupbox ------------
	local grbox = tab4:add_groupbox("Дата и время", 0) -- caption, text_align
	grbox:position(10, 10)
	grbox:size(150, 50)

	------- label with icon -------
	local label_icon = tab4:add_label(0x00000003)
	label_icon:position(10, 70)
	label_icon:set_icon([[toolbar\cool.dll]], 25)

	-------- label with text --------
	local label_text = tab4:add_label(0x00001000 + 0x1)
	label_text:position(label_icon:position() + 5 + label_icon:size(), 70)
	label_text:set_text("Static Text")

	------- progress bar ------------
	--args: vertical = false, hasborder = false, smooth = false, smoothrevers = false
	local prog = tab4:add_progress(false, false, false, false)
	prog:position(10, 95)
	prog:size(250, 10)
	prog:set_progress_pos(40)

	--------- trackbar --------------
	local TrackBarID = 777
	-- local trbar1 = gui.trackbar( ) -- style, id
	local trbar1 = tab4:add_trackbar(0x20 + 0x200, TrackBarID)
	trbar1:position(10, 115)
	trbar1:size(250, 20)

	----- checkbox -----------------
	local Checkbox1_ID = 33
	local chbox = tab4:add_checkbox("checkbox", Checkbox1_ID, false) -- caption, id, treestate
	chbox:position(175, 15)
	callbacks[Checkbox1_ID] = function(state)
		label_text:set_text('checkbox state:'..chbox:check())
	end
	tab4:tooltip(Checkbox1_ID, "checkbox simple", true)
	chbox:check(1)

	----- treestate ---------------
	local Checkbox2_ID = 34
	local chbox3 = tab4:add_checkbox("threestate", Checkbox2_ID, true) -- caption, id, treestate
	chbox3:position(chbox:position(), 35)
	callbacks[Checkbox2_ID] = function(state)
		label_text:set_text('check3state:'..chbox3:check())
	end
	tab4:tooltip(Checkbox2_ID, "checkbox 3-state", true)

	----- radio buttons -----
	local RadioBtn1_ID = 44
	local rbtn = tab4:add_radiobutton("radio_1", RadioBtn1_ID, true) -- caption, id, auto
	rbtn:position(chbox:position(), 55)
	rbtn:check(1)
	callbacks[RadioBtn1_ID] = function(state)
		label_text:set_text('radio_1_clicked')
	end

	local RadioBtn2_ID = 55
	local rbtn2 = tab4:add_radiobutton("radio_2", RadioBtn2_ID, true)
	rbtn2:position(chbox:position(), 75)
	callbacks[RadioBtn2_ID] = function(state)
		label_text:set_text('radio_2_clicked')
	end

	---- editbox with updown ------
	local editbox = tab4:add_editbox( "text",
	0
	-- + 0x2000
	+ 0x0001
	-- + 0x0002
	+ 0x0100
	-- + 0x0800
	-- + 0x0004
	-- + 0x1000
	)
	editbox:position(10, 140)
	editbox:size(60, 20)

	-- local updown = gui.updown(tab4, editbox, 0 -- parent, buddy, style
	local updown = tab4:add_updown(editbox, 0 -- parent, buddy, style
	+ 0x0004
	+ 0x0020
	)
--[[
#define UDS_WRAP                0x0001
#define UDS_SETBUDDYINT         0x0002
#define UDS_ALIGNRIGHT          0x0004
#define UDS_ALIGNLEFT           0x0008
#define UDS_AUTOBUDDY           0x0010 -- bugged
#define UDS_ARROWKEYS           0x0020
#define UDS_HORZ                0x0040
#define UDS_NOTHOUSANDS         0x0080
#define UDS_HOTTRACK            0x0100
]]
	updown:set_range(-50, 50)
	updown:set_current(7)
	updown:on_updown(function(delta) label_text:set_text('udn:'..delta) print(updown:get_current()) end)
	---- buttons -------------
	local Button1_ID = 11
	local btn1 = tab4:add_button("Button1", Button1_ID)
	btn1:position(80, 260)
	btn1:size(45, 25)
	editbox:center_v(btn1)
	callbacks[Button1_ID] = function(state)
		label_text:set_text('btn_1_clicked')
		trbar1:select(30, 60)
	end
	tab4:tooltip(Button1_ID, "кнопка с текстом")

	local Button2_ID = 12
	local btn2 = tab4:add_button("Button2", Button2_ID)
	btn2:position(80 + 5 + btn1:size(), 260)
	btn2:size(45, 25)
	btn2:set_icon([[toolbar\cool.dll]], 59)
	editbox:center_v(btn2)
	callbacks[Button2_ID] = function(state)
		label_text:set_text('btn_2_clicked')
		trbar1:sel_clear()
	end
	tab4:tooltip(Button2_ID, "кнопка с иконкой")
	
	---- listbox ------------------------------
	local ListBoxID = 77
	local listbox = tab4:add_listbox(ListBoxID) -- id, isSorted
	listbox:position(10, 320)
	listbox:size(70, 150)
	for i = 1, 15 do
		listbox:append("line"..i, i*i)
	end
	callbacks[ListBoxID] = function(state)
		--[[
		1 - clicked
		2 - db clicked
		4 - focused
		5 - unfocused
		]]
		if state==2 then
			label_text:set_text("list db clicked:"..listbox:get_line_text(listbox:select()))
		end
		if state == 1 then
			label_text:set_text("list clicked:"..listbox:get_line_data(listbox:select()))
		end
	end
	listbox:remove(2) --ok
	function list_test_menu() label_text:set_text('menu for list clicked') end
	listbox:context_menu{"test|list_test_menu"}

	----- iconed_list -------
	local iconed_list = tab4:add_list(false, false) -- singlesel, multiline
	--iconed_list:set_align("none", 0, false)
	iconed_list:set_iconlib()
	iconed_list:position(10, 165)
	iconed_list:size(120, 150)
	for i = 1, 10 do
		iconed_list:add_item("icon_"..i, nil, i) -- caption, data, icon_idx
	end

	---- treeview -----------
	local tree = tab4:add_tree(0
	 + 0x0001
	 + 0x0002
	 + 0x0004
	--  + 0x4000
	 + 0x0800
	 )
	if colorback then tree:set_tree_colour(colorfore, colorback) end
	tree:position(135, 165)
	tree:size(150, 150)

	local imgcnt = tree:set_iconlib([[toolbar\cool.dll]]) --path [, small_size = true] return count of loaded icons
	-- local imgcnt = tree:set_iconlib([[%windir%\system32\shell32.dll]] )
	local lib1_itm = tree:add_item("cool icons", nil, 16)
	for icon_idx = 0, imgcnt-1 do
		tree:add_item("item_"..icon_idx, lib1_itm, icon_idx, icon_idx, icon_idx )
	end
	tree:tree_set_insert_mode('first')
	local lib2_itm = tree:add_item("sorted", lib1_itm, 16)
	tree:tree_set_insert_mode('sort')
	for icon_idx = 0, 10 do
		tree:add_item("item_"..icon_idx, lib2_itm, icon_idx, icon_idx, icon_idx )
	end
	tree:tree_set_insert_mode(lib2_itm)
	local lib3_itm = tree:add_item("reversed", lib1_itm, 16)
	tree:tree_set_insert_mode('first')
	for icon_idx = 0, 10 do
		tree:add_item("item_"..icon_idx, lib3_itm, icon_idx, icon_idx, icon_idx )
	end

	tree:on_select(function(sel_item)
		label_text:set_text('select tree:'.. tree:tree_get_item_text(sel_item))
	end)

	tree:on_double_click(function(sel_item)
		local data = tree:tree_get_item_data(sel_item)
		label_text:set_text('db click tree:'.. tostring(data))
		label_icon:set_icon([[toolbar\cool.dll]], data)
	end)

	function tree_test_menu()
		label_text:set_text('menu for tree clicked')
	end

	tree:context_menu{"test|tree_test_menu"} -- ok

	---- combolistbox --------
	local ComboBox_ID = 88
	local cbbox = tab4:add_combobox(ComboBox_ID, 0 -- id, style
	+ 0x0003
	+ 0x0040
	) 
	cbbox:position(85, 320)
	cbbox:size(200, 10)
	for i = 1, 10 do
		cbbox:cb_append("combolistbox_"..i)
	end
	cbbox:cb_setcursel(3)
	callbacks[ComboBox_ID] = function(state)
		if state == 1 then
			label_text:set_text(cbbox:get_text())
		end
	end

	---- combobox -----------
	local ComboBox2_ID = 89
	local cbbox = tab4:add_combobox(ComboBox2_ID, 0 -- id, style
	+ 0x0001
	+ 0x0040
	) 
	tab4:add(cbbox, "none")
	cbbox:position(85, 350)
	cbbox:size(200, 120)
	for i=1,10 do
		cbbox:cb_append("line_"..i)
	end
	cbbox:cb_setcursel(1)
	callbacks[ComboBox2_ID] = function(state)
		if state == 1 then
			label_text:set_text(cbbox:get_text())
		end
	end

	---- link -----------------------
	local link = tab4:add_link("For more information <a href=\"http://forum.ru-board.com/\">click here</a>.")
	link:position(10, 480)
	link:size(250, 50)
	
	-------- actions --------
	tab4:on_timer(function() label_timer:set_text(os.date("%x %H:%M:%S")) end)

	tab4:on_scroll( function(id, pos)
		if id == TrackBarID then
			label_text:set_text("scroll to: " .. pos)
			prog:set_progress_pos( pos )
		end
	end)

	tab4:on_command(function(id, state)
		local cb_func = callbacks[id]
		if cb_func then cb_func(state) return true end
		return false
	end)

	tab4:context_menu{"Скрыть панель|"..(IDM_TOOLS + tonumber(props['CN_SIDEBAR']))} -- run IDM_TOOLS+140
	--------------------------------
	-- return {caption = "Test", wnd = tab4, icon_idx = 37}
	tabs:add_tab("Test", tab4, 37) -- caption, wnd, icon_index
	return tab4
end