--[[--------------------------------------------------
	lua_autoformat.lua
	Autoformatter fo Lua scripts
	Authors: Charsi
	Using: https://github.com/Koihik/LuaFormatter
	version 1.0.2
------------------------------------------------------
	Автоформатирование текста Lua скриптов
------------------------------------------------------
	Для подключения добавьте в props строки:

	command.separator.114.$(file.patterns.lua)=1
	command.parent.114.$(file.patterns.lua)=1
	command.name.114.$(file.patterns.lua)=Auto Format
	command.114.$(file.patterns.lua)=dofile $(SciteDefaultHome)\tools\lua_autoformat.lua
	command.mode.114.$(file.patterns.lua)=subsystem:lua,savebefore:no,groupundo:yes
	command.shortcut.114.$(file.patterns.lua)=Alt+F
--]] -------------------------------------------------
local lf_path = [[.\utils\lua-format.exe]]
local cur_file = props['FilePath']:from_utf8(0)
local options = "--indent-width=1 --use-tab --column-limit=300 --line-breaks-after-function-body=1"

local res, err = load(editor:GetText())
if not res then
	print('syntax error:', err)
	return
end

local function get_bookmarks()
	local res = {}
	local line = 0
	while true do
		line = editor:MarkerNext(line, 2)
		if (line == -1) then break end
		res[#res + 1] = {line = line}
		line = line + 1
	end
	return res
end

local s = io.popen(string.format("%s %q %s", lf_path, cur_file, options))
if s then
	local text, qwe = s:read("a")
	s:close()
	if #text > 0 then
		local bookmarks = get_bookmarks()
		local line = editor:LineFromPosition(editor.CurrentPos)
		editor:SelectAll()
		editor:ReplaceSel(text) -- set restyled text
		editor:GotoLine(line)
		for k, v in pairs(bookmarks) do
			-- 			print('reset bookmark', v.line)
			if editor:MarkerGet(v.line, 2) // 2 % 2 ~= 1 then editor:MarkerAdd(v.line, 1) end
		end
		scite.MenuCommand(IDM_SAVE)
	else
		print('LuaFormatter error: invalid options')
	end
end

--[[
OPTIONS:

	  -h, --help                        Display this help menu
	  -v, --verbose                     Turn on verbose mode
	  -i                                Reformats in-place
	  --dump-config                     Dumps the default style used to stdout
	  -c[file], --config=[file]         Style config file
	  --column-limit=[column limit]     Column limit of one line
	  --indent-width=[indentation
	  width]                            Number of spaces used for indentation
	  --tab-width=[tab width]           Number of spaces used per tab
	  --continuation-indent-width=[Continuation
	  indentation width]                Indent width for continuations line
	  --spaces-before-call=[spaces
	  before call]                      Space on function calls
	  --column-table-limit=[column
	  table limit]                      Column limit of each line of a table
	  --table-sep=[table separator]     Character to separate table fields
	  --use-tab                         Use tab for indentation
	  --no-use-tab                      Do not use tab for indentation
	  --keep-simple-control-block-one-line
										keep block in one line
	  --no-keep-simple-control-block-one-line
										Do not keep block in one line
	  --keep-simple-function-one-line   keep function in one line
	  --no-keep-simple-function-one-line
										Do not keep function in one line
	  --align-args                      Align the arguments
	  --no-align-args                   Do not align the arguments
	  --break-after-functioncall-lp     Break after '(' of function call
	  --no-break-after-functioncall-lp  Do not break after '(' of function call
	  --break-before-functioncall-rp    Break before ')' of function call
	  --no-break-before-functioncall-rp Do not break before ')' of function call
	  --align-parameter                 Align the parameters
	  --no-align-parameter              Do not align the parameters
	  --chop-down-parameter             Chop down all parameters
	  --no-chop-down-parameter          Do not chop down all parameters
	  --break-after-functiondef-lp      Break after '(' of function def
	  --no-break-after-functiondef-lp   Do not break after '(' of function def
	  --break-before-functiondef-rp     Break before ')' of function def
	  --no-break-before-functiondef-rp  Do not break before ')' of function def
	  --align-table-field               Align fields of table
	  --no-align-table-field            Do not align fields of table
	  --break-after-table-lb            Break after '{' of table
	  --no-break-after-table-lb         Do not break after '{' of table
	  --break-before-table-rb           Break before '}' of table
	  --no-break-before-table-rb        Do not break before '}' of table
	  --chop-down-table                 Chop down any table
	  --no-chop-down-table              Do not chop down any table
	  --chop-down-kv-table              Chop down table if table contains key
	  --no-chop-down-kv-table           Do not chop down table if table contains
										key
	  --extra-sep-at-table-end          Add extra field separator at end of
										table
	  --no-extra-sep-at-table-end       Do not add extra field separator at end
										of table
	  --break-after-operator            Put break after operators
	  --no-break-after-operator         Do not put break after operators
	  --double-quote-to-single-quote    Transform string literals to use single
										quote
	  --no-double-quote-to-single-quote Do not transform string literals to use
										single quote
	  --single-quote-to-double-quote    Transform string literals to use double
										quote
	  --no-single-quote-to-double-quote Do not transform string literals to use
										double quote
	  --spaces-inside-functiondef-parens
										Put spaces on the inside of parens in
										function headers
	  --no-spaces-inside-functiondef-parens
										Do not put spaces on the inside of
										parens in function headers
	  --spaces-inside-functioncall-parens
										Put spaces on the inside of parens in
										function calls
	  --no-spaces-inside-functioncall-parens
										Do not put spaces on the inside of
										parens in function calls
	  --spaces-inside-table-braces      Put spaces on the inside of braces in
										table constructors
	  --no-spaces-inside-table-braces   Do not put spaces on the inside of
										braces in table constructors
	  --spaces-around-equals-in-field   Put spaces around the equal sign in
										key/value fields
	  --no-spaces-around-equals-in-field
										Do not put spaces around the equal sign
										in key/value fields
	  --line-breaks-after-function-body=[num breaks]
										Line breakes after function body
	  --line-separator=[line separator] input(determined by the input content),
										os(Use line ending of the current
										Operating system), lf(Unix style "\n"),
										crlf(Windows style "\r\n"), cr(classic
										Max style "\r")
	]]
