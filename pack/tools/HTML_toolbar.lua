--[[--------------------------------------------------
HTML_toolbar.lua
Authors: Charsi
Version: 1.1
------------------------------------------------------
Description:
HTML tabbar implemented with SciTE's userstrip
------------------------------------------------------
Connection:
 Set in a file .properties:
command.parent.118.*=11
command.name.118.*=HTML toolbar
command.118.*=dofile $(SciteDefaultHome)\tools\html_toolbar.lua
command.mode.118.*=subsystem:lua,savebefore:no
--]]--------------------------------------------------

local ini = props['scite.userhome'].."\\HTML_toolbar.ini"
local t = {}
event("OnStrip"):clear():register(
function(e, id, state)
	local changeNames = {'unknown', 'clicked', 'change', 'focusIn', 'focusOut'}
	if state == 1 then
		if id == 25 then scite.Open(ini) return end -- btn '?'
		local sel_value = t[id]
		if not sel_value then
			print("can't find insert text for btn with id '"..id.."'")
			scite.Open(ini)
			return
		end
		local new_pos = 0
		editor:ReplaceSel(sel_value
		:gsub('%%...%%', {['%SEL%'] = editor:GetSelText(), ['%CLP%'] = shell.getclipboardtext()}, 2)
		:gsub('()¦', function(pos) new_pos = editor.SelectionStart + pos - 1 return '' end, 1)
		:from_utf8(editor.CodePage))
		if new_pos > 0 then editor:GotoPos(new_pos) end
		gui.pass_focus()
		return
	end
	print('control '..id..' '..changeNames[state+1])
end)

local s = "!"
local idx = 0
local sep = "' • '"
for line in io.lines(ini) do
	if line == '' then
		s = s..sep
	else
		local btn_name, ins = line:match("([%w%s]+)=(.+)")
		t[idx] = ins
		s = s.."( "..btn_name.." )"
	end
	idx = idx+1
end
scite.StripShow(s..sep.."( ? )")
for i = idx, 0,-1 do
	if t[i] then scite.StripSetBtnTipText(i,t[i]) end
end
scite.StripSetBtnTipText(25,"Открыть ini файл")