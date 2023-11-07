-- Замена отступов в виде табуляторов на пробелы и наоборот
-- соответствие количества пробелов заменяющих знак табуляции берется из ваших установок в .properties
-- mozers™ icq#256106175

local sel_text = editor:GetSelText()
if sel_text == '' then
	line_start = 0
	line_end = editor.LineCount-1
else
	line_start = props["SelectionStartLine"] - 1
	line_end = props["SelectionEndLine"] - 1
end

local indent_char = nil
editor:BeginUndoAction()
for line_num = line_start, line_end do
	local len = editor.LineIndentation[line_num]
	if len ~= 0 then
		local line = editor:GetLine(line_num)
		if indent_char == nil then
			indent_char = string.sub(line, 1, 1)
		end
		local shift_end = 0
		if indent_char == "\t" then
			indent = string.rep (" ", len)
		else
			shift_end = len % editor.Indent
			indent = string.rep ("\t", (len-shift_end)/editor.Indent)
		end
		editor.TargetStart = editor:PositionFromLine(line_num)
		editor.TargetEnd = editor.LineIndentPosition[line_num] - shift_end
		editor:ReplaceTarget(indent)
	end
end
editor.Indent = props["indent.size"]
editor:EndUndoAction()  