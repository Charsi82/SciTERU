--[[--------------------------------------------------
InsertSpecialChar.lua
Authors: mozers™, Charsi
Version: 0.7
------------------------------------------------------
Вставка спецсимволов (©,®,§,±,…) из раскрывающегося списка (для HTML вставляются их обозначения)
--]]--------------------------------------------------
local char2html = {
	' ', '&nbsp;',
	'&', '&amp;',
	'"', '&quot;',
	'<', '&lt;',
	'>', '&gt;',
	'‘', '&lsquo;',
	'’', '&rsquo;',
	'“', '&ldquo;',
	'”', '&rdquo;',
	'‹', '&lsaquo;',
	'›', '&rsaquo;',
	'«', '&laquo;',
	'»', '&raquo;',
	'„', '&bdquo;',
	'‚', '&sbquo;',
	'·', '&middot;',
	'…', '&hellip;',
	'§', '&sect;',
	'©', '&copy;',
	'®', '&reg;',
	'™', '&trade;',
	'¦', '&brvbar;',
	'†', '&dagger;',
	'‡', '&Dagger;',
	'¬', '&not;',
	'­', '&shy;',
	'±', '&plusmn;',
	'µ', '&micro;',
	'‰', '&permil;',
	'°', '&deg;',
	'€', '&euro;',
	'¤', '&curren;',
	'•', '&bull;',
}

event("OnStrip"):clear():register(
function(e, id, state)
	local changeNames = {'unknown', 'clicked', 'change', 'focusIn', 'focusOut'}
	if state == 1 then
		local idx = id*2 + (editor.Lexer == SCLEX_HTML and 2 or 1)
		local sel_value = char2html[idx]
		editor:ReplaceSel(sel_value:from_utf8(editor.CodePage))
		return
	end
	print('control '..id..' '..changeNames[state+1])
end)

local s = ""
local function fix_btn_name(txt)
	if txt == ' ' then return '(space)' end
	if txt == '&' then return '(&&)' end
	return "( "..txt.." )"
end
for i=1,#char2html,2 do s = s..fix_btn_name(char2html[i]) end
scite.StripShow(s)
