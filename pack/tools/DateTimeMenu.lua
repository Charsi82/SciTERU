--[[
	Create InsertDateTime Menu items
]]

local start_id = 199

local formats = {
	"%H:%M:%S ",
	"%d %B %Y ",
	"%d %b %Y ",
	"%Y-%m-%d ",
	"%d.%m.%y ",
	"%d/%m/%Y ",
}

if props['locale.properties']:find('locale%-ru%.properties') then
	os.setlocale ("ru", "time")
end

for id, v in ipairs(formats) do
	local command_id = start_id + id
	props['command.parent.'.. command_id ..'.*'] = 41
	props['command.mode.'.. command_id ..'.*'] = 'subsystem:lua,savebefore:no,clearbefore:no'
	props['command.name.'.. command_id ..'.*'] = os.date(v):to_utf8(0)
	props['command.'.. command_id ..'.*'] = [[
	dostring local str = os.date("]]..v..[[") 
	if editor.CodePage==65001 then str = str:to_utf8(0) end
	editor:AddText(str, 65001)
	]]
end