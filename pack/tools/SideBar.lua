--[[--------------------------------------------------
SideBar.lua
Authors: Frank Wunderlich, mozers™, VladVRO, frs, BioInfo, Tymur Gubayev, ur4ltz, nicksabaka
Version 1.30.0
------------------------------------------------------
  Note: Require gui.dll
               lpeg.dll
              shell.dll
             COMMON.lua (function GetCurrentWord)
             eventmanager.lua (function AddEventHandler)

  Connection:
   In file SciTEStartup.lua add a line:
      dofile (props["SciteDefaultHome"].."\\tools\\SideBar.lua")

   Set in a file .properties:
    command.checked.17.*=$(sidebar.show)
    command.name.17.*=SideBar
    command.17.*=SideBar_ShowHide
    command.mode.17.*=subsystem:lua,savebefore:no

    # Set window(1) or pane(0) (default = 0)
    sidebar.win=1

    # Set show(1) or hide(0) to SciTE start
    sidebar.show=1

    # Set sidebar width and position
    sidebar.width=250 (default = 230)
    sidebar.position=left or right (default = right)

    # Set default settings for Functions/Procedures List
    sidebar.functions.flags=1
    sidebar.functions.params=1

    # Set abbrev preview 1-calltip, 0-annotation(default)
    sidebar.abbrev.calltip=1

    # Set annotation style
    style.*.255=fore:#808080,back:#FEFEFE
--]]--------------------------------------------------
require 'gui'
require 'shell'

local panel_width = tonumber(props['sidebar.width']) or 326
local win_height = tonumber(props['position.height']) or 600
local sidebar_position = (props['sidebar.position'] == 'left') and 'left' or 'right'
----------------------------------------------------------
local sb_window = gui.window "Side Bar"
sb_window:size(panel_width + 24, 600)
sb_window:on_close(function() props['sidebar.show'] = 0 end)
if #props['sidebar.posx']>0 and #props['sidebar.posy']>0 then
	sb_window:position(tonumber(props['sidebar.posx']) or 0, tonumber(props['sidebar.posy']) or 0)
end
sb_window:on_move(function() props['sidebar.posx'], props['sidebar.posy'] = sb_window:position() end)
local sb_panel = gui.panel(panel_width)
local base_panel = gui.panel(panel_width)

local style = props['style.*.32']
local colorback = style:match('back:(#%x%x%x%x%x%x)')
local colorfore = style:match('fore:(#%x%x%x%x%x%x)') or '#000000'
----------------------------------------------------------
-- local tabs = gui.tabbar(base_panel)
local tabs = base_panel:add_tabbar('top')
if shell.fileexists(props['sidebar.iconed.tabs']) then
	tabs:set_iconlib(props['sidebar.iconed.tabs'])
end

local tools_path = props["SciteDefaultHome"].."\\tools\\"
for _, file_name in ipairs(gui.files(tools_path.."SideBar_tab*.lua")) do
	local res = dofile(tools_path..file_name)
	local isOK, res = pcall(res, tabs, panel_width, colorback, colorfore)
	if isOK then
		if res then base_panel:client(res) end
	else
		print(res)
	end
end

----------------------------------------------------------
-- Events
----------------------------------------------------------
tabs:on_select(function(ind)
	props['sidebar.active.tab'] = ind
	event('sb_tab_selected')(ind)
	gui.pass_focus()
end)
----------------------------------------------------------

if tonumber(props['sidebar.win']) == 1 then
	sb_window:client(base_panel)
else
	sb_panel:client(base_panel)
end

local function OnSwitchTab()
	tabs:select_tab(tonumber(props['sidebar.active.tab']) or 0)
end

local function SideBar_Show()
	if tonumber(props['sidebar.win']) == 1 then
		sb_window:show()
	else
		gui.set_panel(sb_panel, sidebar_position)
	end
	OnSwitchTab()
end

function SideBar_ShowHide()
	if tonumber(props['sidebar.show']) == 1 then
		props['sidebar.show'] = 0
		if tonumber(props['sidebar.win']) == 1 then
			sb_window:hide()
		else
			gui.set_panel()
		end
	else
		props['sidebar.show'] = 1
		SideBar_Show()
	end
end

local function ResetTabsMenu()
	if tonumber(props['sidebar.win']) == 1 then
		tabs:context_menu{} -- clear menu for window mode
	else
		if props['sidebar.position'] == 'left' then
			tabs:context_menu {
				'Панель справа|MoveSideBarRight',
			}
		else
			tabs:context_menu {
				'Панель слева|MoveSideBarLeft',
			}
		end
	end
end

function SideBar_SwitchMode()
	local sb_shown = tonumber(props['sidebar.show']) == 1
	if tonumber(props['sidebar.win']) == 1 then
-- 		if sb_shown then sb_window:hide() end
		if tonumber(props['sidebar.win']) == 1 then
			props['sidebar.win'] = 0
			sb_window:remove_child(base_panel)
			sb_panel:client(base_panel)
		end
		props['sidebar.win'] = 0
		if sb_shown then sb_window:hide() SideBar_Show() end
	else
-- 		if sb_shown then gui.set_panel() end
		if tonumber(props['sidebar.win']) ~= 1 then
			props['sidebar.win'] = 1
			sb_panel:remove_child(base_panel)
			sb_window:client(base_panel)
		end
		props['sidebar.win'] = 1
		if sb_shown then gui.set_panel() SideBar_Show() end
	end
	ResetTabsMenu()
end

function MoveSideBarLeft()
	if tonumber(props['sidebar.win']) ~= 1 then
		props['sidebar.position'] = 'left'
		sidebar_position = 'left'
		gui.set_panel()
		gui.set_panel(sb_panel, 'left')
		ResetTabsMenu()
	end
end

function MoveSideBarRight()
	if tonumber(props['sidebar.win']) ~= 1 then
		props['sidebar.position'] = 'right' 
		sidebar_position = 'right'
		gui.set_panel()
		gui.set_panel(sb_panel, 'right')
		ResetTabsMenu()
	end
end
------------------------------------------
ResetTabsMenu()
-- now show SideBar:
if tonumber(props['sidebar.show'])==1 then
	AddEventHandler("OnInit", SideBar_Show, 'RunOnce')
end
