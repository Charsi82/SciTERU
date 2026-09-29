--[[--------------------------------------------------
Script Unbound2 Lexer
Authors: Charsi
Version: 0.1
------------------------------------------------------
Description: lexer for Unbound2 (LESTA markup language)
--]] --------------------------------------------------


-- match 0xFFFFFF or 0xFFFFFFFF or int or float
local function isNumber(s)
	local pos = s:match("^0x%x%x%x%x%x%x%x%x()")or s:match("^0x%x%x%x%x%x%x()")
	if pos then return pos - 1 end
	pos = s:match("^%-?%d+%.?%d*()")
	if pos then return pos - 1 end
	return 0
end

local function SetLevel(line_num, level, fold)
	local foldlevel = level | SC_FOLDLEVELBASE
	if fold then
		foldlevel = foldlevel | SC_FOLDLEVELHEADERFLAG
		-- if level~=0 then editor.FoldExpanded[line_num] = true end
	end
	if editor.FoldLevel[line_num]~=foldlevel then editor.FoldLevel[line_num] = foldlevel end
end

local function GetLevel(line_num)
	local count = 0
	local line = editor:GetLine(line_num)
	line:gsub(".", function(ch)
		if ch == '(' then
			count = count + 1
		elseif ch == ')' then
			count = count - 1
		end
	end)
	return count
end

local function UnboundFold()
	if tonumber(props['fold']) ~= 1 then return end
	if tonumber(props['fold.script_unbound']) ~= 1 then return end
	
	local current_level = 0
	for i = 0, editor.LineCount - 1 do
		local count = GetLevel(i)
		if count > 0 then
			SetLevel(i, current_level, true)
		else
			SetLevel(i, current_level, false)
		end
		current_level = current_level + count
	end
	
end

local function UnboundLexer(styler)
	-- local timer = os.clock()
	local S_DEFAULT = 0
	local S_COMMENT = 1
	local S_OPERATOR = 2
	local S_STRING = 3
	local S_NUMBER = 4
	local S_CONTROLLERS = 6
	local S_KEYWORD = 7
	local S_KEYWORD2 = 8
	local S_KEYWORD3 = 5
	local S_KEYWORD4 = 9

	local kw_data = {}
	for _, word in ipairs(props['keywords.$(file.patterns.unbound)']:split()) do kw_data[word]=S_KEYWORD end
	for _, word in ipairs(props['keywords2.$(file.patterns.unbound)']:split()) do kw_data[word]=S_KEYWORD2 end
	for _, word in ipairs(props['keywords3.$(file.patterns.unbound)']:split()) do kw_data[word]=S_KEYWORD3 end
	for _, word in ipairs(props['keywords4.$(file.patterns.unbound)']:split()) do kw_data[word]=S_KEYWORD4 end

	styler:StartStyling(styler.startPos, styler.lengthDoc, styler.initStyle)
	local function isSpace(c) return c == ' ' or c == '\t' or c == '\n' or c == '\r' end
	while styler:More() do
		local c = styler:Current()
		styler:SetState(S_DEFAULT)
		if isSpace(c) then -- skip spaces
		elseif c == '#' then
			styler:SetState(S_COMMENT)
			while styler:More() and not styler:AtLineEnd() do styler:Forward() end

		elseif c == '"' or c == "'" then
			styler:SetState(S_STRING)
			styler:Forward()
			while styler:More() and (styler:Current() ~= c) do
				if (styler:Current() == '#') and isSpace(styler:Next()) then
					styler:SetState(S_COMMENT)
					styler:Forward()
					while styler:More() and not styler:AtLineEnd() do styler:Forward() end
					styler:SetState(S_STRING)
				end
				styler:Forward()
			end
			
		elseif c == '$' then
			styler:SetState(S_CONTROLLERS)
			styler:Forward()
			while styler:More() and styler:Next():match'%w' do styler:Forward() end

		elseif c == '(' or c == ')' or c == '=' or c == ',' or c == ':'
			or c == '{' or c == '}' or c == '[' or c == ']' then
			styler:SetState(S_OPERATOR)
		else
			local sp = styler.Position()
			local cur_line = editor:LineFromPosition(sp)
			local pos_eol = editor.LineEndPosition[cur_line]
			local line = editor:textrange(sp, pos_eol)
			local s = line:match('^[%w_%-%.%%]+')
			if s then
				local len = isNumber(s)
				if len > 0 then
					styler:SetState(S_NUMBER)
					-- for i = 1, len - 1 do styler:Forward() end
					styler:ForwardN(len-1)
				else
					local state = kw_data[s]
					if state then styler:SetState(state) end
					-- for i = 1, #s - 1 do styler:Forward() end
					styler:ForwardN(#s-1)
				end
			end
		end

		styler:Forward()
	end
	styler:EndStyling()
	-- print('time styler', (os.clock() - timer) * 1000, 'ms')
end

AddEventHandler("OnOpen", function()
	if props.Language ~= "script_unbound" then return end
	UnboundFold()
	editor:Colourise(0, -1)
end)

AddEventHandler("OnChar", function()
	if props.Language ~= "script_unbound" then return end
	UnboundFold()
	editor:Colourise(0, -1)
end)

AddEventHandler("OnStyle", function(styler)
	if styler.language == "script_unbound" then
		-- print('dbg:', editor, styler.startPos, styler.lengthDoc, styler.initStyle)
		UnboundLexer(styler)
	end
end)
