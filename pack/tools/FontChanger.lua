--[[--------------------------------------------------
FontChanger.lua
Authors: mozers™, codewarlock1101
Version: 1.1.1
------------------------------------------------------
Смена текущих установок шрифта
C блеском заменяет Ctrl+F11.
Действует одновременно на все отрытые буфера, циклически переключая заданные наборы шрифтов
Можно задать любое количество комбинаций шрифтов
------------------------------------------------------
Для подключения добавьте в свой файл .properties наборы необходимых шрифтов (font.0.*, font.1.*, font.2.*,...)
--]]--------------------------------------------------

local function FontChange(num)
	if props["font.set"] == "" then
		props["font.set"] = "0"
	end
	local nxt_font = props["font.set"] + num
	if props["font."..nxt_font..".base"] == "" then
		nxt_font = "0"
	end
	props["font.set"] = nxt_font
	props["font.base"] = props["font."..nxt_font..".base"]
	props["font.small"] = props["font."..nxt_font..".small"]
	props["font.comment"] = props["font."..nxt_font..".comment"]
end

-- Добавляем свой обработчик события, возникающего при вызове пункта меню "Use Monospaced Font"
event("OnMenuCommand"):register(function(e, msg)
	if msg == IDM_NEXTFONTSET then
		FontChange(1)
	end
end)

-- Восстанавливает набор шрифтов при запуске редактора
event("OnInit"):register(function() FontChange(0) end)
