-------------------------
-- Abbrevs Tab for SideBar
-------------------------
return function(tabs, panel_width, colorback, colorfore)

	local tab3 = gui.panel(panel_width)
	-- local list_ascii = gui.list(true, true) -- multi column, single select, large_icons
	-- tab3:client(list_ascii)
	local list_ascii = tab3:add_list(true, true)
	list_ascii:set_align() -- "client"
	list_ascii:add_column("DEC", 20)
	list_ascii:add_column("HEX", 20)
	list_ascii:add_column("Символ", 20)
	list_ascii:add_column("HTML Номер", 20)
	list_ascii:add_column("HTML Код", 20)

	if colorback then list_ascii:set_list_colour(colorfore, colorback) end
	-------------------------
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
	-------------------------
	tabs:add_tab("ASCII", tab3, 21) -- caption, wnd, icon_index
	return tab3
end