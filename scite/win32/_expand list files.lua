--[[

Find file '*.vcxproj' in current folder.
Create backup *.vcxproj as *_bak.vcxproj.
Expand 
    <ClCompile Include="..\..\lexilla\lexers\*.cxx" />
as list files
    <ClCompile Include="..\..\lexilla\lexers\LexA68k.cxx" />
    <ClCompile Include="..\..\lexilla\lexers\LexAbaqus.cxx" />
    <ClCompile Include="..\..\lexilla\lexers\LexAda.cxx" />
and save result as *.vcxproj.

Requirements: shell.dll
]]

require 'shell'
local cur_path = debug.getinfo(1,"S").short_src:match(".+\\")
print('current path: ', cur_path)
local projects = shell.findfiles( cur_path:to_utf8(0).."*.vcxproj" ) or {}
print('founded', #projects, 'files')
local project_name = projects[1] and projects[1].name
-- if true then return end
if not project_name then
	print("file '*.vcxproj' not found")
	return
else
	print('name =', project_name)
end

-- path to SciTE project file
local path = cur_path..project_name

--------------------------------
local dir = path:match".+\\"
local res = {}
local f = io.open(path)

--create backup
print("create backup...")
local path_back = path:gsub("%.vcx","_bak%1",1)
local fb = io.open(path_back,"wb")
fb:write(f:read("*a"))
fb:close() -- close write
f:close() -- close read
print("create backup done...")

function reduce_dir(s)
	if not s:find("%.%.\\") then return s end
	return reduce_dir(s:gsub("[^\\]-\\%.%.\\",""))
end

fb = io.open(path_back) -- read
for line in fb:lines() do
	if not line:find("%<%!%-%-") and line:find("%*%.") then
		table.insert(res,"<!--" .. line .. "-->")
		local subline = line:match('"(.+)"')
		local mask = dir .. subline
		-- print("mask = ", mask)
		mask = reduce_dir( mask )
		-- print("rd mask = ",mask)
		-- print("subline = ", subline)
		local fmask = shell.findfiles( mask ) or {}
		for k, fn in ipairs(fmask) do
			local str = '"'..subline:gsub("%*%..+$",fn.name,1)..'"'
			str = line:gsub("\"(.+)\"", str)
			-- print(str)
			table.insert(res, str)
		end
	else
		table.insert(res, line)
	end
end
fb:close() -- close read

print("saving result...")
local text = table.concat(res,"\n"):gsub("<PropertyGroup Label=\"Globals\">","%1\r\n    <ProjectName>SciTE</ProjectName>"):gsub(
"  <PropertyGroup Label=\"UserMacros\" />",[[
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <Import Project="SciTERU.props" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="SciTERU.props" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <Import Project="SciTERU.props" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="SciTERU.props" />
  </ImportGroup>
%1]])
-- :gsub([[;%.%.\lua\src]], [[;..\..\lualib\src]], 1):
--[=[gsub([[  </ItemDefinitionGroup>]],[[    <PostBuildEvent>
      <Command>copy "$(TargetPath)" "$(SolutionDir)../../pack\"</Command>
    </PostBuildEvent>
  </ItemDefinitionGroup>]], 1)]=]
f = io.open(path,"wb")
f:write(text)
f:close()
print("done.")
