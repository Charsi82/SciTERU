local function OnPaste()
	local txt = shell.getclipboardtext()
	local tabs = {}
	local _, cnt = txt:gsub("\n\r?([\032\t]*)", function(tab) tabs[#tabs + 1] = #tab end)
	if cnt == 0 then return end
	local _, begin_spaces = txt:find("^[\032\t]*")
	local line = editor:LineFromPosition(editor.SelectionStart)
	local start_line = editor:PositionFromLine(line)
	local indent_before = math.min(editor.LineIndentation[line], (editor.SelectionStart - start_line) * editor.Indent)

	AddEventHandler("OnUpdateUI", function()
		local liness = editor:LineFromPosition(editor.SelectionStart)
		local indent_after = editor.LineIndentation[liness]
		local w = editor.UseTabs and props['tabsize'] or 1
		begin_spaces = begin_spaces * w
		for i = 1, cnt do editor.LineIndentation[line + i] = tabs[i] * w - indent_after + indent_before + begin_spaces end
	end, true)
end


AddEventHandler("OnKey", function(key, shift, ctrl, alt, char)
	if editor.Focus then
		if (ctrl and key == 86) or -- KEY_V
			(shift and key == 45)  -- KEY_Insert
		then
			OnPaste()
		end
	end
end)

AddEventHandler("OnMenuCommand", function(msg, source)
	if editor.Focus and msg == IDM_PASTE then
		OnPaste()
	end
end)
