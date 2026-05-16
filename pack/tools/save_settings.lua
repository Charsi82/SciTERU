--[[--------------------------------------------------
 Save SciTE Settings
 Version: 1.7.3
 Author: mozers™, Dmitry Maslov
---------------------------------------------------
 Save current settings on SciTE close.
 Сохраняет текущие установки при закрытии SciTE (в файле SciTE.session)
---------------------------------------------------
Connection:
In file SciTEStartup.lua add a line:
  dofile (props["SciteDefaultHome"].."\\tools\\save_settings.lua")
Set in a file .properties:
  save.settings=1
  import home\SciTE.session
--]]----------------------------------------------------

local text = ''

-- установить в text текущее значение проперти key
local function SaveKey(key)
	local value = props[key]
	if #value>0 then
		local subst = 0
		text, subst = text:gsub('(%s'..key..'=)%S+', "%1"..value, 1)
		if subst == 0 then text = text ..'\n'..key..'='..value end
	end
end

local function SaveSettings()
	local file = props["scite.userhome"]..'\\SciTE.session'
	if not pcall(io.input, file) then return end
	text = io.read('*a')
	
	local ini_file = props["scite.userhome"]..'\\props_settings.ini'
	local ini = gui.ini_file(ini_file)
	ini:set_section('main')
	local tbl_keys = ini:get_keys()
	for _, key in ipairs(tbl_keys) do
		SaveKey(key)
	end
	if pcall(io.output, file) then
		io.write(text)
	end
	io.close()
end

--[[local function CheckChange(prop_name)
	local prop_value = tonumber(props[prop_name])
	if prop_value==0 then
		props[prop_name] = '1'
	elseif prop_value==1 then
		props[prop_name] = '0'
	end
end]]

-- Добавляем свой обработчик события OnMenuCommand
-- При изменении параметров через меню, меняются и соответствующие значения props[]
AddEventHandler("OnMenuCommand", function(cmd, source)
	if cmd == IDM_VIEWTOOLBAR then
		CheckChange('toolbar.visible')
	elseif cmd == IDM_VIEWTABBAR then
		CheckChange('tabbar.visible')
	elseif cmd == IDM_VIEWSTATUSBAR then
		CheckChange('statusbar.visible')
	elseif cmd == IDM_VIEWSPACE then
		CheckChange('view.whitespace')
	elseif cmd == IDM_VIEWEOL then
		CheckChange('view.eol')
	elseif cmd == IDM_VIEWGUIDES then
		CheckChange('view.indentation.guides')
	elseif cmd == IDM_LINENUMBERMARGIN then
		CheckChange('line.margin.visible')
	elseif cmd == IDM_SPLITVERTICAL then
		CheckChange('split.vertical')
	elseif cmd == IDM_WRAP then
		CheckChange('wrap')
	elseif cmd == IDM_WRAPOUTPUT then
		CheckChange('output.wrap')
	end
end)

-- Добавляем свой обработчик события OnFinalise
-- Сохранение настроек при закрытии SciTE
AddEventHandler("OnFinalise", function()
	if tonumber(props['save.settings']) == 1 then
		SaveSettings()
	end
end)
