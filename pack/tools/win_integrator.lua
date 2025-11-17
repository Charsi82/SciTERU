--[[--------------------------
----  Windows Integrator  ----
--]] -------------------------
require 'gui'
require 'shell'
local winreg = require 'winreg'
local EmptyValue = "empty"
local current_lng = "ru"
local def_associations = {"h", "cpp", "cxx", "php", "lua"}

do
	-- check for admin
	local hkey = winreg.openkey("HKEY_LOCAL_MACHINE\\SOFTWARE", "w")
	if not hkey then
		shell.msgbox("Для изменения настроек\n требуется запуск SciTE\nс правами Администратора!", "Ошибка", 0)
		return
	end
	hkey:close()
	-- generator for controls IDs
	local id = 0
	function next_id()
		id = id + 1
		return id
	end
end

-- win version
local winold = true
do
	local f = io.popen("ver")
	winold = (tonumber(f:read("a"):match("%d+")) or 6) < 10
	f:close()
end

local function get_backup_key(mode)
	-- local reg_backup = 'HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\SciTE\\Script\\WinIntegrator'
	mode = mode or "r"
	local hkey = winreg.openkey("HKEY_LOCAL_MACHINE\\SOFTWARE") -- avoid using WOW6432Node
	return hkey:createkey('SciTE\\Script\\WinIntegrator', mode .. "32")
end

local function regwrite(key, item, val)
	local hkey = winreg.createkey(key, "w")
	if hkey then
		if val == nil then
			hkey:deletevalue(item)
		else
			hkey:setvalue(item, val)
		end
		hkey:close()
	else
		print('can\'t open hkey:', key)
	end
end

local function regdelelete_key(key, delname)
	local hkey = winreg.openkey(key)
	if hkey then
		for name in hkey:enumkey() do
			if name == delname then
				hkey:deletekey(delname)
				break
			end
		end
		hkey:close()
	else
		print('can\'t open hkey:', key)
	end
end

local wnd = gui.window("SciTE Windows Integrator 5.03")
local wnd_w, wnd_h = 400, 430
wnd:size(wnd_w, wnd_h)
local desktop = gui.desktop()
desktop:center(wnd)
local callbacks = {}
local SetLang

local function set_status(txt) wnd:status_setpart(0, txt .. '.') end

-- переключалка интерфейса
local panel_lang = gui.panel(wnd:size())
wnd:add(panel_lang, "top", 50)

local grbox1 = panel_lang:add_groupbox(' Язык интерфейса: ')
grbox1:position(5, 5)
grbox1:size(wnd_w - 30, 40)

-- интеграция в проводник
local panel_expl = gui.panel(wnd:size())
wnd:add(panel_expl, "top", 175)
local grbox2 = panel_expl:add_groupbox(' Интеграция в Проводник: ')
grbox2:position(5, 5)
grbox2:size(wnd_w - 30, 165)

-- интеграция в Windows
local panel_3 = gui.panel(wnd:size())
wnd:add(panel_3, "top", 100)
local grbox3 = panel_3:add_groupbox(' Интеграция в Windows: ')
grbox3:position(5, 5)
grbox3:size(wnd_w - 30, 90)

local RadioBtn1_ID = next_id()
-- local RadioBtn1 = wnd:add_radiobutton("Английский", RadioBtn1_ID, true) -- caption, id, auto
local RadioBtn1 = panel_lang:add_radiobutton("Английский", RadioBtn1_ID, true) -- caption, id, auto
RadioBtn1:position(20, 20)
callbacks[RadioBtn1_ID] = function() SetLang('eng') end
local RadioBtn1_tt = panel_lang:add_tooltip(RadioBtn1_ID) -- CtrlID[, bBaloonStyle = false]

local RadioBtn2_ID = next_id()
-- local RadioBtn2 = wnd:add_radiobutton("Русский", RadioBtn2_ID, true) -- caption, id, auto
local RadioBtn2 = panel_lang:add_radiobutton("Русский", RadioBtn2_ID, true) -- caption, id, auto
RadioBtn2:position(155, 20)
callbacks[RadioBtn2_ID] = function() SetLang('ru') end
local RadioBtn2_tt = panel_lang:add_tooltip(RadioBtn2_ID)

if props['locale.properties'] == 'locale-ru.properties' then
	RadioBtn2:check(1)
else
	RadioBtn1:check(1)
end

local btnOK_ID = next_id()
local btnOK = wnd:add_button(btnOK_ID, "OK")
btnOK:position(155, 330)
btnOK:size(70, 25)
wnd:center_h(btnOK)
callbacks[btnOK_ID] = function() wnd:close() end

local cb3_posy = 50
local label1 = panel_expl:add_label(0, "Связать файлы заданных расширений с SciTE:")
label1:position(15, cb3_posy - 25)
label1:size(200, 20)

local ComboBox1_ID = next_id()
local cbbox = panel_expl:add_combobox(ComboBox1_ID, 0 -- id, style
+ 0x0002 + 0x0040 + 0x0100)
-- wnd:add(cbbox, "none")
cbbox:position(15, cb3_posy)
cbbox:size(80, 20)
local ComboBox1_tt = panel_expl:add_tooltip(ComboBox1_ID)

do
	local hkey = get_backup_key()
	if hkey then
		for key in hkey:enumvalue() do if not key:find("^associations") then cbbox:cb_append(key) end end
		cbbox:cb_setcursel(0)
		hkey:close()
	end
end

local function RegSessionPath()
	regwrite("HKCR\\SciTE.Session\\DefaultIcon", '', props['SciteDefaultHome'] .. "\\SciTE.exe,2")
	regwrite("HKCR\\SciTE.Session\\shell\\open\\command", '', 'cmd /v:on /c "set "file=%1" && start "" "' .. props['SciteDefaultHome'] .. [[\SciTE.exe" -check.if.already.open=0 "-loadsession:!file:\=\\!"]])
	regwrite("HKCR\\Applications\\SCITE.EXE\\shell\\edit\\command\\", '', props['SciteDefaultHome'] .. '\\SciTE.exe "%1"')
end

-- при закрытии окна принудительно обновляем записи реестра с путями, т.к. они путь к папке с программой мог измениться
local function ApplyAssociations()
	regwrite("HKCR\\SciTE.File", '', "SciTE file")
	regwrite("HKCR\\SciTE.File\\DefaultIcon", '', props["SciteDefaultHome"] .. "\\SciTE.exe,1")
	regwrite("HKCR\\SciTE.File\\shell\\open\\command", '', props["SciteDefaultHome"] .. '\\SciTE.exe "%1"')
	RegSessionPath()
end

local function AddAssoc(ext)
	local hkey = get_backup_key("rw")
	local old_association = hkey:getstrval(ext)
	if old_association then
		if current_lng == "ru" then
			set_status("Расширение '" .. ext .. "' уже используется")
		else
			set_status("Association '" .. ext .. "' already used")
		end
		return
	end
	local hkey_ext = winreg.openkey("HKCR\\." .. ext, "rw")
	if hkey_ext then
		local current_association = hkey_ext:getstrval('')
		hkey_ext:setvalue('', 'SciTE.File')
		hkey_ext:close()
		hkey:setvalue(ext, current_association or EmptyValue)
	else
		regwrite("HKCR\\." .. ext, '', 'SciTE.File')
		hkey:setvalue(ext, EmptyValue)
	end
	hkey:close()
	if current_lng == "ru" then
		set_status("Расширение '" .. ext .. "' добавлено")
	else
		set_status("Association '" .. ext .. "' registered")
	end
end

local function RemoveAssoc(ext)
	local hkey = get_backup_key("rw")
	local old_association = hkey:getstrval(ext)
	hkey:deletevalue(ext)
	hkey:close()
	local hkey_ext = winreg.openkey("HKCR\\." .. ext, "w")
	if not hkey_ext then return end
	if old_association == EmptyValue or old_association == nil then
		hkey_ext:deletevalue('')
		if current_lng == "ru" then
			set_status("Расширение '" .. ext .. "' удалено")
		else
			set_status("Association '" .. ext .. "' removed")
		end
	else
		hkey_ext:setvalue('', old_association)
		if current_lng == "ru" then
			set_status("Расширение '" .. ext .. "' восстановлено " .. old_association)
		else
			set_status("Association '" .. ext .. "' restore to " .. old_association)
		end
	end
	hkey_ext:close()
end

local btnADD_ID = next_id()
local btnADD = panel_expl:add_button(btnADD_ID, "+")
local btnADD_tt = panel_expl:add_tooltip(btnADD_ID)
btnADD:position(110, cb3_posy)
btnADD:size(20, 20)
callbacks[btnADD_ID] = function()
	local text = cbbox:get_text()
	local idx = cbbox:cb_find(text)
	if idx == -1 and #text > 0 then
		cbbox:cb_append(text)
		AddAssoc(text)
	end
end

local btnDEL_ID = next_id()
local btnDEL = panel_expl:add_button(btnDEL_ID, "-")
local btnDEL_tt = panel_expl:add_tooltip(btnDEL_ID)
btnDEL:position(140, cb3_posy)
btnDEL:size(20, 20)
callbacks[btnDEL_ID] = function()
	local text = cbbox:get_text()
	local idx = cbbox:cb_find(text)
	if idx > -1 then
		RemoveAssoc(text)
		cbbox:cb_remove(idx)
		cbbox:cb_setcursel(0)
	end
end

local ResetBtn_ID = next_id()
local btn_reset = panel_expl:add_button(ResetBtn_ID, "Default")
local ResetBtn_tt = panel_expl:add_tooltip(ResetBtn_ID)
btn_reset:position(170, cb3_posy)
btn_reset:size(90, 20)

callbacks[ResetBtn_ID] = function()
	local cnt = cbbox:cb_count()
	for i = 1, cnt do RemoveAssoc(cbbox:cb_item_text(i - 1)) end
	cbbox:cb_clear()
	for _, ext in ipairs(def_associations) do
		AddAssoc(ext)
		cbbox:cb_append(ext)
	end
	cbbox:cb_setcursel(0)
	if current_lng == "ru" then
		set_status('Установлены расширения по умолчанию ' .. table.concat(def_associations, ','))
	else
		set_status('Associations reset to ' .. table.concat(def_associations, ','))
	end
end

local ResetBtnW10_ID = next_id()
local btn_param = panel_expl:add_button(ResetBtnW10_ID, "Параметры")
btn_param:position(270, cb3_posy)
btn_param:size(90, 20)
callbacks[ResetBtnW10_ID] = function()
	shell.exec("ms-settings:defaultapps")
end

cbbox:enable(winold)
btnADD:enable(winold)
btnDEL:enable(winold)
btn_reset:enable(winold)
btn_param:enable(not winold)

local function SetSession()
	regwrite("HKCR\\.session", '', "SciTE.Session")
	regwrite("HKCR\\SciTE.Session", '', "SciTE session file")
	RegSessionPath()
	if current_lng == "ru" then
		set_status("Добавлена ассоциация с расширением '.session'")
	else
		set_status("Set association with '.session'")
	end
	-- cmd /v:on /c "set "file=%1" && start "" "G:\\Program Files (x86)\\SciTE\\SciTE.exe" -check.if.already.open=0 "-loadsession:!file:\=\\!"
end

local function UnsetSession()
	regwrite("HKCR\\.session", '')
	if current_lng == "ru" then
		set_status("Удалена ассоциация с расширением '.session'")
	else
		set_status("Unset association with '.session'")
	end
end

local CheckBox2_ID = next_id()
local CheckBox2 = panel_expl:add_checkbox('Открывать файлы *.session как файлы сессий SciTE', CheckBox2_ID)
local CheckBox2_tt = panel_expl:add_tooltip(CheckBox2_ID)
CheckBox2:position(15, cb3_posy + 30)
CheckBox2:size(300, 20)

callbacks[CheckBox2_ID] = function()
	local state = CheckBox2:check()
	if state == 1 then
		SetSession()
	else
		UnsetSession()
	end
end
local hkey = winreg.openkey("HKCR\\.session")
if hkey and hkey:getvalue("") == "SciTE.Session" then CheckBox2:check(1) end

local CheckBox3_ID = next_id()
local CheckBox3 = panel_expl:add_checkbox('Добавить SciTE в контекстное меню "Отправить"', CheckBox3_ID)
local CheckBox3_tt = panel_expl:add_tooltip(CheckBox3_ID)
CheckBox3:position(15, cb3_posy + 60)
CheckBox3:size(300, 20)
local send_to = gui.get_folder(9)

local function create_link() gui.create_link(props['SciteDefaultHome'] .. "\\SciTE.exe", send_to .. "\\SciTE.lnk") end

local function remove_link() os.remove(send_to:from_utf8(0) .. "\\SciTE.lnk") end

callbacks[CheckBox3_ID] = function()
	local state = CheckBox3:check()
	if state == 1 then
		create_link()
		if current_lng == "ru" then
			set_status("В папку 'Отправить' добавлен ярлык 'SciTE.exe'")
		else
			set_status("Add 'SciTE.exe' link to folder 'Send To'")
		end
	else
		remove_link()
		if current_lng == "ru" then
			set_status("Ярлык 'SciTE.exe' удалён из папки 'Отправить'")
		else
			set_status("Link 'SciTE.exe' removed from folder 'Send To'")
		end
	end
end

local link_exit = shell.fileexists(send_to .. "\\SciTE.lnk")
remove_link()
if link_exit then create_link() end
CheckBox3:check(link_exit and 1 or 0)

local CheckBox4_ID = next_id()
local CheckBox4 = panel_expl:add_checkbox("Установить SciTE в качестве дефолтового HTML редактора", CheckBox4_ID)
local CheckBox4_tt = panel_expl:add_tooltip(CheckBox4_ID)
CheckBox4:position(15, cb3_posy + 90)
CheckBox4:size(350, 20)

callbacks[CheckBox4_ID] = function()
	local value = CheckBox4:check() == 1
	if value then
		regwrite("HKCR\\.htm\\OpenWithList\\SCITE.EXE", '', '')
		regwrite("HKCR\\.html\\OpenWithList\\SCITE.EXE", '', '')
	else
		regdelelete_key("HKCR\\.htm\\OpenWithList", "SCITE.EXE")
		regdelelete_key("HKCR\\.html\\OpenWithList", "SCITE.EXE")
	end
	if current_lng == "ru" then
		set_status(value and "SciTE установлен как редактор HTML" or "SciTE удалён из редакторов HTML")
	else
		set_status(value and "Set SciTE as HTML editor" or "Unset SciTE as HTML editor")
	end
end

local hkey = winreg.openkey("HKCR\\.html\\OpenWithList\\SCITE.EXE")
if hkey then
	CheckBox4:check(1)
	hkey:close()
end

-- Интеграция в Windows
local label2 = panel_3:add_label(0, "Установить SciTE.Helper (COM-сервер для управления SciTE)")
label2:position(15, 25)

-- register Helper
local RegSrvBtn_ID = next_id()
local regbtns_posy = 50
local RegSrvBtn = panel_3:add_button(RegSrvBtn_ID, "Зарегистрировать")
RegSrvBtn:position(50, regbtns_posy)
RegSrvBtn:size(110, 30)
callbacks[RegSrvBtn_ID] = function()
	local cmd = string.format('regsvr32 /s "%s%s"', props['SciteDefaultHome']:from_utf8(0), "\\tools\\Helper\\SciTE.dll")
	local isOK = os.execute(cmd)
	local lng = current_lng == "ru"
	set_status(isOK and (lng and "Регистрация успешна завершена" or "Registeration success") or (lng and "Ошибка регистрации" or "Registeration failed"))
end

-- unregister Helper
local UnRegSrvBtn_ID = next_id()
local UnRegSrvBtn = panel_3:add_button(UnRegSrvBtn_ID, "Удалить")
UnRegSrvBtn:position(210, regbtns_posy)
UnRegSrvBtn:size(110, 30)
callbacks[UnRegSrvBtn_ID] = function()
	local cmd = string.format('regsvr32 /u /s "%s%s"', props['SciteDefaultHome']:from_utf8(0), "\\tools\\Helper\\SciTE.dll")
	local isOK = os.execute(cmd)
	local lng = current_lng == "ru"
	set_status(isOK and (lng and "Отмена регистрация успешна завершена" or "Unregisteration success") or (lng and "Ошибка отмены регистрации" or "Unregisteration failed"))
end

wnd:on_focus(function(focus)
	if focus then
		wnd:remove_transparent()
	else
		wnd:set_transparent(128)
	end
end)

local function OnCommand(cmd_id, ...)
	local cb = callbacks[cmd_id]
	if cb then cb(...) end
end
panel_lang:on_command(OnCommand)
panel_expl:on_command(OnCommand)
panel_3:on_command(OnCommand)
wnd:on_command(OnCommand)
wnd:on_close(ApplyAssociations)
wnd:statusbar(-1)

function SetLang(lng_id)
	if lng_id ~= "eng" and lng_id ~= "ru" then return end
	current_lng = lng_id
	local str_ids = {
		["eng"] = {
			btn_param = "Options",
			rbtn1 = "English",
			rbtn2 = "Russian",
			grbox1 = " Interface Language: ",
			regsrv = "Register",
			unregsrv = "Unregister",
			intgr_expl = " Integration with Explorer: ",
			intgr_win = " Integration with Windows: ",
			label_helper = "Install SciTE.Helper (COM-server for communication with SciTE)",
			-- st_bar = "Administrator rights required.",
			label_bind = "Associate SciTE with extensions (For Win10+ use Options):",
			check_sess = "Associate files *.session as SciTE session files",
			check_sendto = 'Add SciTE to "SendTo" context menu',
			check_html_def = "Set SciTE as default HTML editor",
			lang_tip = "Change interface this application",
			combo1_tip = "You\'ll able to open in SciTE files with this extensions by double click",
			btnreset_text = "Default",
			btnreset_tip = "Reset associations to default",
			check2_tip = "You\'ll able to open SciTE session files by double click",
			check3_tip = "You\'ll able to open any selected files with SciTE",
			check4_tip = 'Adds SciTE to the "Open With.." menu for html files',
			btn_add_tip = 'Add association',
			btn_del_tip = 'Remove association',
			btn_reset_tip = 'Restore default associations (' .. table.concat(def_associations, ',') .. ')'
		},
		["ru"] = {
			btn_param = "Параметры",
			rbtn1 = "Английский",
			rbtn2 = "Русский",
			grbox1 = " Язык интерфейса: ",
			regsrv = "Зарегистрировать",
			unregsrv = "Удалить",
			intgr_expl = " Интеграция в Проводник: ",
			intgr_win = " Интеграция в Windows: ",
			label_helper = "Установить SciTE.Helper (COM-сервер для управления SciTE)",
			-- st_bar = "Требуются права Администратора.",
			label_bind = "Связать расширения с SciTE (Для Win10+ через Параметры):",
			check_sess = "Открывать файлы *.session как файлы сессий SciTE",
			check_sendto = 'Добавить SciTE в контекстное меню "Отправить"',
			check_html_def = "Установить SciTE в качестве дефолтового HTML редактора",
			lang_tip = "Сменить интерфейс этого приложения",
			combo1_tip = "Позволяет открывать в SciTE файлы ассоциированных типов с помощью двойного клика мыши",
			btnreset_text = "По умолчанию",
			btnreset_tip = "Установить ассоциации по умолчанию",
			check2_tip = "Позволяет открывать сохраненные ранее сессиии SciTE",
			check3_tip = "Позволяет открывать в SciTE любые выбранные файлы",
			check4_tip = 'Добавляет SciTE в меню "Открыть с помощью..." для html файлов',
			btn_add_tip = 'Добавить расширение',
			btn_del_tip = 'Удалить расширение',
			btn_reset_tip = 'Восстановить расширения по умполчанию (' .. table.concat(def_associations, ',') .. ')'
		}
	}
	local lng = str_ids[lng_id]
	if lng then
		btn_param:set_text(lng['btn_param'] or '')
		grbox1:set_text(lng['grbox1'] or '')
		RadioBtn1:set_text(lng['rbtn1'] or '')
		RadioBtn1_tt:set_text(lng['lang_tip'] or '')
		RadioBtn2:set_text(lng['rbtn2'] or '')
		RadioBtn2_tt:set_text(lng['lang_tip'] or '')
		ComboBox1_tt:set_text(lng['combo1_tip'] or '')
		btnADD_tt:set_text(lng['btn_add_tip'] or '')
		btnDEL_tt:set_text(lng['btn_del_tip'] or '')
		btn_reset:set_text(lng['btnreset_text'] or '')
		ResetBtn_tt:set_text(lng['btn_reset_tip'] or '')
		grbox2:set_text(lng['intgr_expl'] or '')
		label1:set_text(lng['label_bind'] or '')
		CheckBox2:set_text(lng['check_sess'] or '')
		CheckBox2_tt:set_text(lng['check2_tip'] or '')
		CheckBox3:set_text(lng['check_sendto'] or '')
		CheckBox3_tt:set_text(lng['check3_tip'] or '')
		CheckBox4:set_text(lng['check_html_def'] or '')
		CheckBox4_tt:set_text(lng['check4_tip'] or '')
		grbox3:set_text(lng['intgr_win'] or '')
		label2:set_text(lng['label_helper'] or '')
		RegSrvBtn:set_text(lng['regsrv'] or '')
		UnRegSrvBtn:set_text(lng['unregsrv'] or '')
		set_status(current_lng == "ru" and 'Готово' or 'Ready')
	end
end

SetLang((props['locale.properties'] == 'locale-ru.properties') and 'ru' or 'eng')
wnd:show(true)
