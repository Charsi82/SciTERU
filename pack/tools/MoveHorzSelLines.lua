--[[--------------------------------------------------
MoveHorzSelLines.lua
Authors: Charsi
Version: 0.1
------------------------------------------------------
Description:
   ГОРИЗОНТАЛЬНЫЙ сдвиг строк и блоков выделения.
 Для прямоугольного выделения сдвигаются строки, содержащие выделение.
 Для блочного:
 - если выделение пусто - сдвигается строка с кареткой
 - если выделено несколько блоков - сдвигаются строки, содержащие блоки выделения
 - если начало и конец выделения находятся на одной строке, то сдвигается выделенный блок
 - иначе сдвигаются строки, содержащие выделение
------------------------------------------------------
Connection:
 Set in a file .properties:

   command.name.72.*=Leftwards
   command.72.*=dostring left=true dofile(props["SciteDefaultHome"].."\\tools\\MoveHorzSelLines.lua")
   command.mode.72.*=subsystem:lua,savebefore:no,clearbefore:no
   command.shortcut.72.*=Alt+Left
   
   command.name.73.*=Rightwards
   command.73.*=dostring left=false dofile(props["SciteDefaultHome"].."\\tools\\MoveHorzSelLines.lua")
   command.mode.73.*=subsystem:lua,savebefore:no,clearbefore:no
   command.shortcut.73.*=Alt+Right
--]]--------------------------------------------------

local function AddIndent(t)
	local step = left and -1 or 1
	for _, line in ipairs(t) do editor.LineIndentation[line] = editor.LineIndentation[line] + step end
end

if editor.SelectionIsRectangle then
	local rcar = editor.RectangularSelectionCaret
	local ranch = editor.RectangularSelectionAnchor
	if rcar > ranch then rcar, ranch = ranch, rcar end
	local lines = {}
	for i = editor:LineFromPosition(rcar), editor:LineFromPosition(ranch) do lines[#lines + 1] = i end
	AddIndent(lines)
	return
end

if editor.SelectionEmpty then
	AddIndent {editor:LineFromPosition(editor.CurrentPos)}
	return
end

if editor.Selections > 1 then
	local lines = {}
	local step = left and -1 or 1
	for i = 1, editor.Selections do
		local from = editor:LineFromPosition(editor.SelectionNStart[i - 1])
		local to = editor:LineFromPosition(editor.SelectionNEnd[i - 1])
		for line = from, to do
			if not lines[line] then
				editor.LineIndentation[line] = editor.LineIndentation[line] + step
				lines[line] = true
			end
		end
	end
	return
end

local ss = editor.SelectionStart
local se = editor.SelectionEnd
local sel_start_line = editor:LineFromPosition(editor.SelectionStart)
local sel_end_line = editor:LineFromPosition(editor.SelectionEnd)
if sel_start_line ~= sel_end_line then
	local lines = {}
	for i = sel_start_line, sel_end_line do lines[#lines + 1] = i end
	AddIndent(lines)
	return
end

if left then -- left
	if ss == editor:PositionFromLine(editor:LineFromPosition(editor.CurrentPos)) then
		-- print('at line start')
		return
	end
	local forward = editor.Anchor > editor.CurrentPos
	local count = editor:CountCharacters(ss, se)
	editor:CharLeft()
	editor:CharLeftExtend()
	local sel = editor:GetSelText()
	editor:DeleteBack()
	local nss = editor:PositionBefore(ss)
	local nse = nss
	while editor:CountCodeUnits(nss, nse) < count do nse = nse + 1 end
	editor:InsertText(nse, sel)
	if forward then
		editor:SetSel(nse, nss)
	else
		editor:SetSel(nss, nse)
	end
else -- right
	if se == editor.LineEndPosition[editor:LineFromPosition(editor.CurrentPos)] then
		-- print('at end line')
		return
	end
	local forward = editor.Anchor > editor.CurrentPos
	local count = editor:CountCharacters(ss, se)
	local sel, len = editor:GetSelText()
	editor:ReplaceSel('')
	editor:CharRight()
	editor:InsertText(-1, sel)
	local nss = editor.CurrentPos
	local nse = nss
	while editor:CountCodeUnits(nss, nse) < count do nse = nse + 1 end
	if forward then
		editor:SetSel(nse, nss)
	else
		editor:SetSel(nss, nse)
	end
end
