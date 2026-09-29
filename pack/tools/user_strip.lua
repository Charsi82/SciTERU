--[[
Пользовательская панель

-- создать панель по шаблону str и показать её
scite.StripShow( str )
Шаблон str является последовательностью нескольких паттернов, каждый изкоторых представляет следующее:
- символ ! - на панель справа добавляется кнопка 'закрыть'
- 'Label' - статический текст с надписью Label
- [Editbox] - поле ввода с надписью Editbox
- {Combobox} - выпадающий список с дефолтным значением Combobox. Список заполяется функцией scite.StripSetList(list)
- (Button) - кнопка с надписью Button
- ((OK)) - кнопка, выбранная по умолчанию, с надписью OK
- \n - следующий контрол будет расположен строкой ниже

Создаваемые элементы управления автоматически индексируются, начиная с нуля.
Полученный индекс используется в функции-обработчике

-- закрыть панель
scite.StripShow("")

-- установить значение value для контрола с указанным индексом 
scite.StripSet(index, value)

-- установить список для выпадающего списка с указанным индексом. Стирает дефолтное значение
scite.StripSetList(index, "value1\nvalue2\nvalue3")

-- вернуть значение контрола с указанным индексом
scite.StripValue(index)

-- обработка событий доступна в глобальной функции OnStrip(index, state)
-- index - номер контрола, state - тип изменения (clicked=1, change=2, focusIn=3, focusOut=4)

Добавить в стартовый скрипт строчку
function OnStrip(...) event("OnStrip")(...) end
]]

-- Пример использования:

-- удалить старый обработчик и зарегистрировать новый
event("OnStrip"):clear()
AddEventHandler("OnStrip", function(id, state)
	if id == -1 then 
		-- удаляем обработчик так как панели больше нет
		event("OnStrip"):clear() 
		return 
	end
	-- нажата кнопка в 5 позиции (с надписью Close), то закрываем панель
	if id == 5 and state == 1 then scite.StripShow("") return end
	if id == 2 and state == 1 then print("Button 9 at your service. :)") return end
	local NameState = {'unknown', 'clicked', 'change', 'focusIn', 'focusOut'}
	print('control', id, NameState[state + 1])
end)

-- создать панель
local user_panel = {
	'!', -- добавить в правой части панели крестик для закрытия
	"'Label:'", -- статик с текстом "Label:"
	'[Editbox]', -- поле ввода
	'(Button 9)', -- кнопка
	'\n', -- переход на другую строку
	"'Label2:'", -- статик с текстом "Label2:"
	'{Combobox}', -- выпадающий список
	'((Close))' -- кнопка выбранная по умолчанию
}

scite.StripShow(table.concat(user_panel))
-- устанавливаем всплывающие подсказки
scite.StripSetBtnTipText(5, 'Close')
scite.StripSetBtnTipText(2, 'Hello')
-- заполняем выпадающий список
scite.StripSetList(4,'item1\nitem2\nitem3')
scite.StripSet(4,'item1')