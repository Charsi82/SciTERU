--[[--------------------------------------------------
SortText.lua
Authors: Tugarinov Sergey, mozers™, Tymur Gubayev
version 2.1.1
------------------------------------------------------
Sorting selected lines_tbl alphabetically and vice versa
Сортировка выделенных строк по алфавиту и наоборот

Connection:
	Set in a file .properties:
		command.name.37.*=Sorting of lines A… z / z… A
		command.37.*=dofile $(SciteDefaultHome)\tools\SortText.lua
		command.mode.37.*=subsystem:lua,savebefore:no
--]]--------------------------------------------------

local lines_tbl = {} -- Таблица со строками нашего текста
local sort_direction_decreasing = false -- Обратный порядок сортировки
local patt = [[^[%s'"`«]*]] -- паттерн для сортировки без учета пробелов и кавычек в начале строки

-- сравниваем две строки
local function CompareTwoLines(line1, line2)
	line1 = line1:gsub(patt, '')
	line2 = line2:gsub(patt, '')
	if sort_direction_decreasing then
		return (line1:lower() > line2:lower())
	else
		return (line1:lower() < line2:lower())
	end
end

-- автоматически определяем направление сортировки, последовательно сравнивая строки с последней строкой
local function GetSortDirection()
	local end_line = lines_tbl[#lines_tbl]:gsub(patt, '')
	for i = 1, #lines_tbl-1 do
		local comp_line = lines_tbl[i]:gsub(patt, '')
		if comp_line ~= end_line then
			return CompareTwoLines(comp_line, end_line)
		end
	end
end

local sel_text = editor:GetSelText()
local sel_start = editor.SelectionStart
local sel_end = editor.SelectionEnd
if sel_text ~= '' then
	-- разделяем на строки и загоняем их в таблицу
	for current_line in sel_text:gmatch('[^\n]+') do
		lines_tbl[#lines_tbl+1] = current_line
	end
	if #lines_tbl > 1 then
		sort_direction_decreasing = GetSortDirection()
		-- сортируем строки в таблице
		table.sort(lines_tbl, CompareTwoLines)
		-- соединяем все строки из таблицы вместе
		local out_text = table.concat(lines_tbl, '\n')..'\n'
		editor:ReplaceSel(out_text)
	end
end
-- восстанавливаем выделение
editor:SetSel(sel_start, sel_end)
