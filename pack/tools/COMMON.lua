-- COMMON.lua
-- Version: 1.13.5
---------------------------------------------------
-- Общие функции, использующиеся во многих скриптах
---------------------------------------------------
-- Пути поиска подключаемых lua-библиотек и модулей
package.path = props["SciteDefaultHome"] .. "\\tools\\LuaLib\\?.lua;" .. package.path
package.cpath = props["SciteDefaultHome"] .. "\\tools\\LuaLib\\?.dll;" .. package.cpath
--------------------------------------------------------
-- Подключение пользовательского обработчика к событию SciTE
-- dofile(props["SciteDefaultHome"]..'\\tools\\eventmanager.lua')
dofile(props["SciteDefaultHome"] .. '\\tools\\xs_eventmanager.lua')

-- Функция распознавания URL
dofile(props["SciteDefaultHome"] .. "\\tools\\URL_detect.lua")
--------------------------------------------------------
-- Замена порой неработающего props['CurrentWord']
-- 12.05.2022 перенесено в ядро /*RB_GETCURWRLD*/
-- function GetCurrentWord()
-- 	local current_pos = editor.CurrentPos
-- 	return editor:textrange(editor:WordStartPosition(current_pos, true),
-- 							editor:WordEndPosition(current_pos, true))
-- end

--------------------------------------------------------
--- Returns current hotspot's text
function GetCurrentHotspot()
	local s = editor.CurrentPos
	local e = s + 1
	local l = editor.Length
	s = s - 1 -- we know we're at hotspot
	while editor.StyleHotSpot[editor.StyleAt[s]] and s >= 0 do s = s - 1 end
	while editor.StyleHotSpot[editor.StyleAt[e]] and e <= l do e = e + 1 end
	return editor:textrange(s + 1, e)
end

--------------------------------------------------------
--- Returns current EOL symbol and it's length
function GetEOL()
	local m = editor.EOLMode
	if m == SC_EOL_CRLF then
		return '\r\n', 2
	elseif m == SC_EOL_CR then
		return '\r', 1
	elseif m == SC_EOL_LF then
		return '\n', 1
	end
end

--------------------------------------------------------
-- Замена ф-ций string.lower(), string.upper(), string.len()
-- Работает с любыми национальными кодировками
local function StringLower(s, cp)
	cp = tonumber(cp or editor:codepage())
	if cp == 65001 then return s:utf8lower() end
	return s:to_utf8(cp):utf8lower():from_utf8(cp)
end

local function StringUpper(s, cp)
	cp = tonumber(cp or editor:codepage())
	if cp == 65001 then return s:utf8upper() end
	return s:to_utf8(cp):utf8upper():from_utf8(cp)
end

local function StringLen(s, cp)
	cp = tonumber(cp or editor:codepage())
	if cp == 65001 then return s:utf8len() end
	return s:to_utf8(cp):utf8len()
end

--------------------------------------------------------
-- string.to_pattern возращает строку, пригодную для использования
-- в виде паттерна в string.find и т.п.
-- Например: "xx-yy" -> "xx%-yy"
local lua_patt_chars = "[%(%)%.%+%-%*%?%[%]%^%$%%]" -- управляющие паттернами символов Луа:
function string.pattern(s)
	return (s:gsub(lua_patt_chars, '%%%0')) -- фактически экранирование служебных символов символом %
end

--------------------------------------------------------
-- Проверяет параметр на nil и если это так, то возвращает default, иначе возвращает сам параметр
function ifnil(val, default)
	if val == nil then
		return default
	else
		return val
	end
end

--------------------------------------------------------
-- Определение соответствует ли стиль символа стилю комментария
function IsComment(pos)
	local style = editor.StyleAt[pos]
	local lexer = props['Language']
	local comment = {
		abap = {1, 2},
		ada = {10},
		asm = {1, 11},
		au3 = {1, 2},
		baan = {1, 2},
		bullant = {1, 2, 3},
		caml = {12, 13, 14, 15},
		cpp = {1, 2, 3, 15, 17, 18},
		csound = {1, 9},
		css = {9},
		d = {1, 2, 3, 4, 15, 16, 17},
		escript = {1, 2, 3},
		euphoria = {1, 18},
		flagship = {1, 2, 3, 4, 5, 6},
		forth = {1, 2, 3},
		gap = {9},
		hypertext = {9, 20, 29, 42, 43, 44, 57, 58, 59, 72, 82, 92, 107, 124, 125},
		xml = {9, 29},
		inno = {1, 7},
		latex = {4},
		lua = {1, 2, 3},
		script_lua = {4, 5},
		mmixal = {1, 17},
		nsis = {1, 18},
		opal = {1, 2},
		pascal = {2, 3, 4},
		perl = {2},
		bash = {2},
		pov = {1, 2},
		ps = {1, 2, 3},
		python = {1, 12},
		rebol = {1, 2},
		ruby = {2},
		rust = {1, 2, 3, 4},
		scriptol = {2, 3, 4, 5},
		smalltalk = {3},
		specman = {2, 3},
		spice = {8},
		sql = {1, 2, 3, 13, 15, 17, 18},
		tcl = {1, 2, 20, 21},
		verilog = {1, 2, 3},
		vhdl = {1, 2}
	}

	-- Для лексеров, перечисленных в массиве:
	for l, ts in pairs(comment) do
		if l == lexer then
			for _, s in ipairs(ts) do if s == style then return true end end
			return false
		end
	end
	-- Для остальных лексеров:
	-- asn1, ave, blitzbasic, cmake, conf, eiffel, eiffelkw, erlang, euphoria, fortran, f77, freebasic, kix, lisp, lout, octave, matlab, metapost, nncrontab, props, batch, makefile, diff, purebasic, vb, yaml
	if style == 1 then return true end
	return false
end

------[[ T E X T   M A R K S ]]-------------------------

-- Выделение текста маркером определенного стиля
function EditorMarkText(start, length, indic_number)
	local current_indic_number = scite.SendEditor(SCI_GETINDICATORCURRENT)
	scite.SendEditor(SCI_SETINDICATORCURRENT, indic_number)
	scite.SendEditor(SCI_INDICATORFILLRANGE, start, length)
	scite.SendEditor(SCI_SETINDICATORCURRENT, current_indic_number)
end

-- Очистка текста от маркерного выделения заданного стиля
--   если параметры отсутсвуют - очищаются все стили во всем тексте
--   если не указана позиция и длина - очищается весь текст
function EditorClearMarks(indic_number, start, length)
	local _first_indic, _end_indic
	local current_indic_number = scite.SendEditor(SCI_GETINDICATORCURRENT)
	if indic_number == nil then
		_first_indic, _end_indic = 0, 31
	else
		_first_indic, _end_indic = indic_number, indic_number
	end
	if start == nil then start, length = 0, editor.Length end
	for indic = _first_indic, _end_indic do
		scite.SendEditor(SCI_SETINDICATORCURRENT, indic)
		scite.SendEditor(SCI_INDICATORCLEARRANGE, start, length)
	end
	scite.SendEditor(SCI_SETINDICATORCURRENT, current_indic_number)
end

----------------------------------------------------------------------------
-- Задание стиля для маркеров (затем эти маркеры можно будет использовать в скриптах, вызывая их по номеру)

-- Translate color from RGB to win
local function encodeRGB2WIN(color)
	if string.sub(color, 1, 1) == "#" and string.len(color) > 6 then
		return tonumber(string.sub(color, 6, 7) .. string.sub(color, 4, 5) .. string.sub(color, 2, 3), 16)
	else
		return tonumber(color)
	end
end

local function InitMarkStyle(indic_number, indic_style, indic_color, indic_alpha, indic_outlinealpha)
	editor.IndicStyle[indic_number] = indic_style
	editor.IndicFore[indic_number] = encodeRGB2WIN(indic_color)
	editor.IndicAlpha[indic_number] = indic_alpha
	editor.IndicOutlineAlpha[indic_number] = indic_outlinealpha
end

local function EditorInitMarkStyles()
	local string2value = {
		plain = INDIC_PLAIN,
		squiggle = INDIC_SQUIGGLE,
		tt = INDIC_TT,
		diagonal = INDIC_DIAGONAL,
		strike = INDIC_STRIKE,
		hidden = INDIC_HIDDEN,
		roundbox = INDIC_ROUNDBOX,
		box = INDIC_BOX,
		dotbox = INDIC_DOTBOX,
		straightbox = INDIC_STRAIGHTBOX,
		squigglelow = INDIC_SQUIGGLELOW,
		squigglepixmap = INDIC_SQUIGGLEPIXMAP,
		compthick = INDIC_COMPOSITIONTHICK,
		compthin = INDIC_COMPOSITIONTHIN,
		fullbox = INDIC_FULLBOX,
		pointchar = INDIC_POINTCHARACTER,
		gradient = INDIC_GRADIENTCENTRE
		-- 		hotspot  = INDIC_HOTSPOT
	}
	for indic_number = 0, 31 do
		local mark = props["indic.style." .. indic_number]
		if mark ~= "" then
			local indic_color = mark:match("#%x%x%x%x%x%x") or (props["find.mark"]):match("#%x%x%x%x%x%x") or "#0F0F0F"
			local indic_style = string2value[mark:match("%l+")] or INDIC_ROUNDBOX
			local indic_alpha = tonumber((mark:match("%@%d+") or ""):sub(2)) or 30
			local indic_outlinealpha = tonumber((mark:match("$%d+") or ""):sub(2)) or 50
			InitMarkStyle(indic_number, indic_style, indic_color, indic_alpha, indic_outlinealpha)
		end
	end
end

----------------------------------------------------------------------------
-- Отрисовка вертикальной тонкой линии, отделяющей колонку маркеров фолдинга от текста (для красоты)
local function SetMarginTypeN()
	editor.MarginTypeN[3] = SC_MARGIN_TEXT
	editor.MarginWidthN[3] = 1
end

----------------------------------------------------------------------------
-- Инвертирование состояния заданного параметра (используется для снятия/установки "галок" в меню)
function CheckChange(prop_name)
	local cur_prop = ifnil(tonumber(props[prop_name]), 0)
	props[prop_name] = 1 - cur_prop
end

-- ==============================================================
-- Функция копирования os_copy(source_path,dest_path)
-- Автор z00n <http://www.lua.ru/forum/posts/list/15/89.page>
function os_copy(source_path, dest_path)
	local res, err = shell.fileCopy(source_path, dest_path)
	-- if not res then print(err) end
	--[[	source_path = source_path:from_utf8(0)
	dest_path = dest_path:from_utf8(0)
	-- "библиотечная" функция
	local function unwind_protect(thunk, cleanup)
		local ok, res = pcall(thunk)
		if cleanup then cleanup() end
		if not ok then
			error(res, 0)
		else
			return res
		end
	end
	-- общая функция для работы с открытыми файлами
	local function with_open_file(name, mode)
		return function(body)
			local f, err = io.open(name, mode)
			if err then return end
			return unwind_protect(function() return body(f) end, function() return f and f:close() end)
		end
	end
	----------------------------------------------
	return with_open_file(source_path, "rb")(function(source)
		return with_open_file(dest_path, "wb")(function(dest)
			assert(dest:write(assert(source:read("*a"))))
			return true
		end)
	end)]]
end

function os_rename(old, new)
	-- 	print('os.rename >>> ',os.rename(old:from_utf8(0), new:from_utf8(0)))
	-- 	os.rename(old:from_utf8(0), new:from_utf8(0))
	local res, err = shell.fileRename(old, new)
	-- if not res then print(err) end
end

function os_move(from, to) shell.fileMove(from, to) end

function os_remove(filename, nonconfirm)
	-- 	print('os.remove >>>',os.remove(filename:from_utf8(0)))
	-- 	os.remove(filename:from_utf8(0))
	if nonconfirm == nil then nonconfirm = true end
	local res, err = shell.fileDelete(filename, nonconfirm)
	-- if not res then print(err) end
end

-- ==============================================================
--- Читает файлы .abbrev (понимает инструкцию #import)
-- @return Таблица пар сокращение-расшифровка
function ReadAbbrevFile(file, abbr_table)
	--[[------------------------------------------
	Эмулирует чтение файла внутренней функцией редактора
	Функция предназначена для использования вместо io.lines(filename), а также вместо file:lines()
	Читает файл по правилам SciTE: при наличии в конце строки символа '\' считается, что текущая строка продолжается в следующей.
	@usage: for l in scite_io_lines('c:\\some.file') do print(l) end
	  alternative:
	f = io.open('s:\\some.file')
	for l in scite_io_lines(f) do print(l) end
	--]] ------------------------------------------
	local function scite_io_lines(file)
		local line_iter = type(file) == 'string' and io.lines(file) or file:lines()
		local scite_iter = function()
			local line = line_iter()
			if not line then return end
			-- start [SciTE]
			while string.sub(line, -1) == '\\' do line = string.sub(line, 1, -2) .. line_iter() end
			-- end [SciTE]
			return line
		end
		return scite_iter
	end
	--------------------------------------------
	local abbrev_file, err, errcode = io.open(file)
	if not abbrev_file then return abbrev_file, err, errcode end

	local abbr_table = abbr_table or {}
	local ignorecomment = tonumber(props['abbrev.' .. props['Language'] .. '.ignore.comment']) == 1
	for line in scite_io_lines(abbrev_file) do
		if line ~= '' and (ignorecomment or line:sub(1, 1) ~= '#') then
			local _abr, _exp = line:match('^(.-)=(.+)')
			if _abr then
				abbr_table[#abbr_table + 1] = {abbr = _abr, exp = _exp}
			else
				local import_file = line:match('^import%s+(.+)')
				-- если обнаружена запись import, то рекурсивно вызываем эту же функцию
				if import_file then ReadAbbrevFile(file:match('.+\\') .. import_file, abbr_table) end
			end
		end
	end
	abbrev_file:close()
	return abbr_table
end

-- ==============================================================
-- Функции, выполняющиеся только один раз, при открытии первого файла
--   ( Выполнить их сразу, при загрузке SciTEStartup.lua, нельзя
--   получим сообщение об ошибке: "Editor pane is not accessible at this time." )
AddEventHandler("OnOpen", function()
	string.lower = StringLower
	string.upper = StringUpper
	string.len = StringLen
	string.trim = function(s) return string.match(s, '^%s*(.*%S)') or '' end
	EditorInitMarkStyles()
	SetMarginTypeN()
	props["pane.accessible"] = '1'
end, 'RunOnce')

AddEventHandler("OnMenuCommand", function(msg, source)
	if msg == IDM_FILTER then
		if props["old.find.handler"] == "1" then
			dofile("tools\\FindText.lua", props["findtext.first.mark"])
			return true
		end
	end
end)

function LSTBC()
	local links = {}
	local text = shell.getclipboardtext()
	for s in text:gmatch("[0-9A-Z]+") do table.insert(links, "https://lesta.ru/shop/redeem/?bonus_mode=" .. s) end
	local text = table.concat(links, "\n")
	print(text)
	editor:CopyText(text)
end

function run_script(path)
	local secs = tonumber(props['lua.runtime.quota']) or 0
	if secs > 0 then
		local st = os.clock()
		function check()
			if os.clock() - st > secs then
				debug.sethook() -- disable hooks
				error("runtime quota in " .. secs .. " seconds exceeded", 2)
			end
		end
		debug.sethook(check, '', 100000)
	end
	local res, err = pcall(dofile, path)
	if not res then print(err) end
	if secs > 0 then debug.sethook() end
end

function PathCollapse(s) return (s:gsub(props['SciteDefaultHome'], "$SciteDefaultHome", 1)) end

function PathExpand(s) return (s:gsub("$SciteDefaultHome", props['SciteDefaultHome'], 1)) end
