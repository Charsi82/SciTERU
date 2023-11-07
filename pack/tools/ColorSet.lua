--[[--------------------------------------------------
ColorSet.lua
Authors: mozers™
version 1.3
------------------------------------------------------
  Connection:
   Set in a file .properties:
     command.name.6.*=Choice Color
     command.6.*=dofile $(SciteDefaultHome)\tools\ColorSet.lua
     command.mode.6.*=subsystem:lua,savebefore:no

  Note: Needed gui.dll
--]]-------------------------------------------------- #ff80ff
require 'gui'

local colour = props["CurrentSelection"]
local prefix, clr = colour:match("(#?)([0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F])$")
local low = false
if not clr then
	prefix, clr = colour:match("(#?)[0-9a-f]([0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f])$")
	if clr then
		low = true
	else
		prefix = "#"
		colour = "#FFFFFF"
	end
end

colour = gui.colour_dlg(colour)
if colour then
	if prefix == '' then colour = colour:gsub('^#', '') end
	if low then colour = colour:lower() end
	editor:ReplaceSel(colour)
end
