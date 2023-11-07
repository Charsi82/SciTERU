-------------------------
-- ASCII Tab for SideBar
-------------------------
return function(tabs, panel_width, colorback, colorfore)

	local tab2 = gui.panel(panel_width)
	-- local list_abbrev = gui.list(true)
	-- tab2:client(list_abbrev)
	local list_abbrev = tab2:add_list(true)
	list_abbrev:set_align()
	list_abbrev:add_column("Сокращения", 60)
	list_abbrev:add_column("Расширение", 600)

	if colorback then list_abbrev:set_list_colour(colorfore, colorback) end
	-- Переключатель способа предпросмотра аббревиатур: true = calltip, false = annotation
	local Abbreviations_USECALLTIPS = tonumber(props['sidebar.abbrev.calltip']) == 1
	-------------------------
	local cur_fn = ""
	local function Abbreviations_ListFILL()
		local abbrev_filename = props['AbbrevPath']
		if abbrev_filename == cur_fn then return end
		cur_fn = abbrev_filename
		local abbr_table = ReadAbbrevFile(abbrev_filename)
		list_abbrev:clear()
		if not abbr_table then return end
		for i,v in ipairs(abbr_table) do
			list_abbrev:add_item({v.abbr, v.exp:gsub('\t', '\\t'):gsub('%%%%', '%%')}, v.exp)
		end
		list_abbrev:autosize(0)
	end

	local Abbreviations_HideExpansion = Abbreviations_USECALLTIPS and
	function() editor:CallTipCancel() end or
	function() editor:AnnotationClearAll() end

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

	local function OnSwitch()
		if tab2:bounds() then -- visible Abbrev
			Abbreviations_ListFILL()
		end
	end

	AddEventHandler("OnSwitchFile", OnSwitch)
	AddEventHandler("OnOpen", OnSwitch)
	AddEventHandler("OnSave", OnSwitch)
	event('sb_tab_selected'):register(function(e, tab_id) if tab_id == 2 then OnSwitch() end end)
	-------------------------
	tabs:add_tab("Сокращения", tab2, 22) -- caption, wnd, icon_index
	return tab2
end