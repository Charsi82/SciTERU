--[[-----------------------------------------------------------------
xs_eventmanager.lua
Authors: Charsi
version: 1.0.1
---------------------------------------------------------------------
  Description:
	simple event manager realization for SciTE.
	exported functions (self-descriptive):
	
	@ params:
	@ EventName as string
	@ Handle as function
	[@RunOnce] as bool
	
	* AddEventHandler ( EventName, Handler[, RunOnce] ) => Handler
	
	Менеджер событий для SciTE
  
Require: events.dll

  Подключение:
	не требуется (загружается из COMMON.lua).
  Если же по каким-либо причинам необходимо подключение вручную, то:
	В файл SciTEStartup.lua добавьте строку:
	dofile (props["SciteDefaultHome"].."\\tools\\xs_eventmanager.lua")
	(перед подключением скриптов, использующих AddEventHandler)
	
  Описание:
	AddEventHandler оставлен для совместимости со старыми скриптами.
	Также можно в скриптах создавать и запускать свои произвольные события.
	
	Обработчик:
	
	@ params:
	@ event событие, для которого был вызван обработчик
	@ ... дополнительные аргументы
	fnHandler = function( event, ... )
		print("hello from event ' .. event:name()))
		-- ... some other actions ...
		-- event:removeThisCallback() -- if handler run once
		-- return true --  if nees stop event. same as e:stop()
	end
	
	Событие:
	
	Cоздает или возвращает объект события с указанным именем
	event( "sEventName" )
	
	Регистрация функции-обработчика fnHandler ( подписка на событие )
	Обработчик помещается в начало очереди на выполенние, если bToBegin не ложь.
	event( "sEventName" ):register( fnHandler [,bToBegin])
	
	Отрегистрация функции-обработчика fnHandler ( отписка от события )
	event( "sEventName" ):unregister( fnHandler )
	
	Запуск события с передачей аргументов для обработчиков
	event( "sEventName" ):trigger( arg1, arg2, ... )
	event( "sEventName" )( arg1, arg2, ... )
	
	Возвращает имя события
	event( "sEventName" ):name() -- > "sEventName"
	
	Останавливает обработку события
	event( "sEventName" ):stop()
	
	Установка маркера перед запуском события. Сбрасывается после отработки события.
	event( "sEventName" ):setFingerprint( 'debug_123' )

	Отрегистрация текущего обработчика.
	event( "sEventName" ):removeThisCallback()
	
	Отрегистрация всех обработчиков. Если событие запущено,то оно останавливается.
	event( "sEventName" ):clear()
	
	Печатает информацию событии.
	event( "sEventName" ):print()
	
	Метод trigger возвращает логическое значение (true - если нужно прервать обработку события).
	Остальные методы возвращают объект события, для которого они были вызваны.
---------------------------------------------------------------------
History:
	* 1.0.0 initial release
	* 1.0.1 handler can be added to the front of the queue
--]]-----------------------------------------------------------------

require 'events'

function AddEventHandler(EventName, Handler, RunOnce)
	event(EventName):register(function(e, ...)
		if RunOnce then e:removeThisCallback() end
		return Handler(...)
	end)
	if not _G[EventName] then
		_G[EventName] = function(...) return event(EventName)(...) end
	end
end