--[[--------------------------------------------------
lexer_name.lua
Authors: mozers™, VladVRO
version 1.1.4
------------------------------------------------------
Показ имени текущего лексера в строке статуса

Подключение:
В файл SciTEStartup.lua добавьте строку:
  dofile (props["SciteDefaultHome"].."\\tools\\lexer_name.lua")
включите scite.lexer.name в статусную строку:
  statusbar.text.1=Line:$(LineNumber) Col:$(ColumnNumber) [$(scite.lexer.name)]
--]]--------------------------------------------------

local last_lexer
local function SetPropLexerName()
	if props['FileName'] == '' then return end
	local cur_lexer = props['Language']
	if cur_lexer ~= last_lexer then
		props["scite.lexer.name"] = (cur_lexer == "hypertext") and "html" or cur_lexer
		last_lexer = cur_lexer
	end
end

-- Добавляем свой обработчик события OnLanguage
AddEventHandler("OnLanguage", SetPropLexerName)

-- Добавляем свой обработчик события OnSwitchFile
AddEventHandler("OnSwitchFile", SetPropLexerName)
