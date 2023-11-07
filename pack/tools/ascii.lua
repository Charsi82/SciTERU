-- Трассировка выделенного текста в коды ASCII
-- Автор: gansA <gans_a@newmail.ru>
-----------------------------------------------
local str = editor:GetSelText()
if editor.CodePage == 65001 then str = str:from_utf8(0) end
if #str>0 then print('>char HEX  DEC') end
str:gsub('.', function(strS)
    local strD = string.byte(strS, 1)
    print(string.format(' [%s] %02X %d', strS, strD, strD))
end)


