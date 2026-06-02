-- тест скриптового тулбара
local wnd = gui.window("test_toolbar")
wnd:position(250, 250)
wnd:size(500, 200)
-- requires for break main loop
wnd:on_close(function() wnd:quit() end)

-- main panel
local panel = gui.panel()
panel:set_background("#67C77A")
wnd:client(panel)

-- toolbar
local toolbar_panel = gui.panel()
local btn_size = 24
wnd:add(toolbar_panel, "top", btn_size, false)
local callbacks = {}

for i = 1, 55 do
	local btn = toolbar_panel:add_button()
	btn:position((i - 1) * btn_size + 1, 0)
	btn:size(btn_size, btn_size)
	btn:set_icon([[toolbar\cool.dll]], i - 1)
	toolbar_panel:set_tiptext(btn:get_ctrl_id(), "tiptext for " .. i .. " button", '', false)
	callbacks[btn:get_ctrl_id()] = function(...) print('btn_' .. i .. '_clicked', ...) end
end

toolbar_panel:on_command(function(id, state)
	local cb_func = callbacks[id]
	if cb_func then
		cb_func(state)
		return true
	end
	return false
end)

-- show and run
wnd:show()
wnd:loop()
print('done')
