--[[
Подсчет контрольной сумы для выделенного текста или содержимого файла, если выделение это путь до него
Используются функции библиотеки shell.dll
shell.calc_sha256( text )
shell.calc_md5( text [, check_filepath = false])
]]
require 'shell'

local args = {...}
local mode = args[1] == 'md5'
local check_filepath = args[2] == true
		
local function GetOpenFilePath(text)
	-- Example: $(SciteDefaultHome)\tools\RestoreRecent.js
	local pattern_sci = '^$[(](.-)[)]'
	local res, cnt = text:gsub(pattern_sci, function(p) return props[p] end, 1)
	if cnt>0 then return res end
	
	-- Example: %APPDATA%\Opera\Opera\profile\opera6.ini
	local pattern_env = '^[%%](.-)[%%]'
	local res, cnt = text:gsub(pattern_env, function(p) return os.getenv[p] end, 1)
	if cnt>0 then return res end

	-- Example: props["SciteDefaultHome"].."\\tools\\Zoom.lua"
-- 	local pattern_props = '^props%[%p(.-)%p%]%.%.%p(.*)%p'
-- 	local scite_prop1, scite_prop2 = text:match(pattern_props)
	local pattern_props = '^props%[(%p)(.-)%1%]%.%.(%p)(.*)(%3)'
	local _,scite_props,_,fn,_ = text:match(pattern_props)
	if scite_props and fn then
		return props[scite_props]..fn
	end
	return text
end

local sel = editor:GetSelText()
if check_filepath then sel = GetOpenFilePath(sel) end
if mode then
	print('MD5:', shell.calc_md5(sel, check_filepath) )
else
	if check_filepath and shell.fileexists(sel) then
		local file = io.open( sel, "rb")
		print('SHA-256:', shell.calc_sha256(file:read('a*')) )
		file:close()
	else
		print('SHA-256:', shell.calc_sha256(sel) )
	end
end