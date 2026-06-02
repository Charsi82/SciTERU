--[[--------------------------------------------------
Script Text Lexer
Authors: Tymur Gubayev
Version: 1.1.2
------------------------------------------------------
Description:	text-"lexer": colors latin chars, 
	national chars and highlights links. There's
	also possibility to 
--]] --------------------------------------------------

local function TextLexer(styler)
	local S_DEFAULT = 0
	local S_IDENTIFIER = 1
	local S_LINK = 2
	local S_NATIONAL_CHARS = 3

	local IsIdentifier = function (c)
		return c:find('^[%a_][%w_]*')
	end

	local IsLink = IsURI or function(c)
		return c:find('[:%w_&%?.%%-@%$%+=%*~%/]')
	end

	local national_chars = props['chars.accented']
	local IsNational_Char = function(c)
		if #national_chars == 0 then return false end
		return c:find('[' .. national_chars .. ']') -- @todo: this wont work well for UTF
	end

	-- print("Styling: ", styler.startPos, styler.lengthDoc, styler.initStyle)
	styler:StartStyling(styler.startPos, styler.lengthDoc, styler.initStyle)
	local styler_endPos = styler.startPos + styler.lengthDoc

	while styler:More() do
		local stst = styler:State()
		local c = styler:Current()
		-- Exit state if needed
		if stst == S_NATIONAL_CHARS and not IsNational_Char(c) then
			styler:SetState(S_DEFAULT)
			--[[--links are processed at once, so the state cannot be LINK
			elseif stst == LINK and not IsLink(c) then
			styler:SetState(DEFAULT)]]
		end

		local n -- link special var
		-- Enter state if needed
		if styler:State() == S_DEFAULT then
			local sp = styler.Position()
			local cur_line = editor:LineFromPosition(sp)
			local pos_eol = editor.LineEndPosition[cur_line]
			local s = editor:textrange(sp, pos_eol)
			n = IsLink(s)
			if n then
				-- print(n,s:sub(1,n),'\n\t',s)
				styler:SetState(S_LINK)
				for i = 1, n do styler:Forward() end
				styler:SetState(S_DEFAULT)
			elseif IsNational_Char(c) then
				styler:SetState(S_NATIONAL_CHARS)
			else
				local idb, ide = IsIdentifier(s)
				if idb then
					styler:SetState(S_IDENTIFIER)
					for i = 0, ide - idb do styler:Forward() end
					styler:SetState(S_DEFAULT)
				end
			end
		end

		-- if current text is a link, styler:Forward() is already called
		if not n then styler:Forward() end
	end
	styler:EndStyling()
end

AddEventHandler("OnStyle", function(styler)
	if styler.language == "script_text" then 
		TextLexer(styler)
	end
end)

AddEventHandler("OnHotSpotReleaseClick", function(ctrl)
	if not ctrl and props['Language'] == "script_text" then
		local URL = GetCurrentHotspot()
		-- check if URL is like "a@b.c"
		if URL:find('^%w+@') then URL = "mailto:" .. URL end
		shell.exec(URL)
	end
	return true
end)
