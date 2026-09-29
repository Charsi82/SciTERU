-- load 'astyle' function from shell.dll
require "shell"

if type(astyle) == 'function' then
    local text = editor:GetText()
    local pos = editor.CurrentPos
    editor:SelectAll()
    editor:ReplaceSel(astyle(text, props["astyle.options"])) -- set restyled text
    editor:SetSel(pos, pos)
    scite.MenuCommand(IDM_SAVE)
else
    print("Can't find astyle() function.")
end
