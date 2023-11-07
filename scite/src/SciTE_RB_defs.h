#pragma once
/*
SciTE Ru-Board additions defines
*/
//-------------------------------------------------------------------------

// [BatchLexerImprovement]
#define RB_BALI

// [BetterCalltips]
#define RB_BTCT1 // Ctrl + Up/Down for switch tooltip pages
#define RB_BTCT2 // "calltip.*.automatic" disable autopopup tooltip
// not implemented
//"calltip.*.show.per.page"
//"calltip.*.word.wrap"
#define RB_BTCT3 // Fix colorize calltip

// [BufferNumber]
#define RB_BUFFNUMBER

// [CalltipBreaks]
#define RB_CTBR

// [CheckFileExist]
#define RB_CFE

// [CheckMenus]
#define RB_CheckMenus

// [EditorUnicodeMode]
#define RB_EUM

// [EncodingToLua]
// export strings functions for utf8 
#define RB_UTF8

// [English_KeyCode]
#define RB_ENKEY

// [ErrorLineBack]
#define RB_ELB

// [EventInvalid]
// disabled LuaExtension::OnSave
//#define RB_EVINV //?

// [ExtendedContextMenu]
#define RB_ECM

// [FileAttr in PROPS]
#define RB_FAINP

// [FindResultListStyle]
#define RB_FRLS

// [FixEncoding][EncodingToLua]
#define RB_ENCODING

// [FixFind]
#define RB_FixFind

// [FixReplaceOnce]
// fixed in vanilla

// [ForthImprovement]
#define RB_FRIM

// [GetApplicationProps]
#define RB_GAP

// [GetWordChars]
#define RB_GWC

// [GoMessageFix]
#define RB_GMFIX

// [GoMessageImprovement]
#define RB_GMI

// [InMultiWordsList]
#define RB_IMWL

// [InputErr]
#define RB_IERR

// [InsertAbbreviation]
#define RB_IA

// [InsertMultiPasteText]
#define RB_IMLTPT

// [LangMenuChecker]
#define RB_LangMenuChecker

// [LexersFoldFix]
#define RB_LFF

// [LexersLastWordFix]
#define RB_LLWF

// [LocalizationFromLua]
#define RB_LFL

// [LuaLexerImprovement]
// del

// [MoreRecentFiles]
#define RB_MoreRecentFiles

// [MouseClickHandled]
#define RB_MCH

// [NewBufferPosition]
#define RB_NBP

// [NewFind-MarkerDeleteAll]
#define RB_NFMDA

// [OnClick]
#define RB_ONCLICK

// [OnDoubleClick]
#define RB_ODBCLK

// [OnFinalise]
#define RB_FIN

// [OnHotSpotReleaseClick]
#define RB_OHSC

// [OnKey]
#define RB_ONKEY

// [OnMenuCommand]
#define RB_OMC

// [OnMouseButtonUp]
#define RB_OMBU

// [OnSendEditor]
#define RB_OnSendEditor

// [OpenNonExistent]
#define RB_ONE

// [ParametersDialogFromLua]
#define RB_PDFL

// [Perform]
// export editor.Perform(...)
#define RB_Perform

// [PropsColouriseFix]
#define RB_PCF
 
// [PropsKeysSets]
#define RB_PKS

// [PropsKeywords]
#define RB_PKW

// [ReadOnlyTabMarker]
#define RB_ROTM

// [ReloadStartupScript]
#define RB_RSS

// [ReturnBackAfterRALL]
#define RB_RBAFALL

// [SaveEnabled]
#define RB_SE

// [SetBookmark]
#define RB_SB

// [StartupScriptReload]
// add scite.ReloadStartupScript()
#define RB_SSR

// [StyleDefHotspot]
#define RB_HOTSPOT

// [StyleDefault]
#define RB_SD

// [SubMenu]
#define RB_SUBMENU

// [TabbarTitleMaxLength]
#define RB_TTML

// [TabsMoving]
#define RB_TM

// [ToolsMax]
#define RB_ToolsMax

// [UserListItemID]
#define RB_ULID

// [UserPropertiesFilesSubmenu]
#define RB_UserPropertiesFilesSubmenu

// [VBLexerImprovement]
#define RB_VBLI

// [WarningMessage]
#define RB_WRNM

// [Zoom]
#ifdef RB_OnSendEditor
#define RB_ZOOM
#endif

// [ZorderSwitchingOnClose]
#ifdef RB_GAP
#define RB_ZSOC
#endif

// [autocompleteword.incremental]
#define RB_ACI

// [caret]
#define RB_CARET

// [close_on_dbl_clk]
#define RB_TAB_DB_CLICK

// [cmdline.spaces.fix]
//#define RB_CLSF // fixed

// [find.fillout]
#define RB_FINDFILL

// [find_in_files_no_empty]
#define RB_FFNOE

// [fix_invalid_codepage]
#define RB_FIXCP

// [ignore_overstrike_change]
#define RB_IOCH

// [import]
#define RB_IMPORT

// [macro]
#define RB_MACRO

// [new_on_dbl_clk]
#define RB_NODBCLK

// [oem2ansi]
#define RB_OEM2ANSI

// [output.caret]
#define RB_OUTCARET

// [save.session.multibuffers.only]
#define RB_SSMO

// [scite.userhome]
#define RB_SUH

// [selection.hide.on.deactivate] -> "selection.always.visible"

// [session.close.buffers.onload]
#define RB_SCBO

// [update.inno]
#define RB_UPIN

// [user.toolbar]
#define RB_UT

// [utf8.auto.check]
#define RB_UTF8AC

// [warning.couldnotopenfile.disable]
#define RB_WNRSUPRESS

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// открывает файл, вместо ярлыка файла
#define RB_LINK

// [user_shortcuts]
#define RB_USERSC

// добавляет экспорт события перемещения вкладок
#define RB_ONTABMOVE

// полоска над активной вкладкой
#define RB_TABTOP

// обработка скроллинга при курсоре над панелью вкладок
#define RB_TABMA

// альтернативная иконка главного окна просле 354-го дня года (SciTExmas.ico)
#define RB_DOY

// [clear_before_execute]
// оверрайд clearbefore:[1,0,yes,no] глобальной настройки clear.before.execute для конкретной команды
#define RB_CBE

// props['GetCurrentWord']
#define RB_GETCURWORD

// скрывает список автозавершения при перемещении окна
#define RB_ACCANCELONMOVE

// для пользовательских горячих клавиш позволяет задавать 'Num *' вместо 'KeypadMultiply' и т.д.
#define RB_USCNUM

// скрывает панель фильтра при переключении вкладок props["hide.filterstrip.on.switch.tab"]
#define RB_HFS

// автопрокрутка экрана с сохранением положения активной строки
#define RB_MCIVEX

// всплывающая подсказка для кнопок пользовательской панели
#define RB_USBTT

// перерисовка окон редактора и консоли при перемещении сплиттера
#define RB_ONMOVESPL

// текст в описании программы
#define RB_CREDITS

// добавляет проверку видимости при установке значений в элементы управления пользовательской панели
#define RB_USVC

// не добавляем хоткеи в меню из закомментированных строк в конфиг файле (устарело - см. RB_PCMF)
// #define RB_HKFIX

// fix LuaExtension::OnExecute
#define RB_LEONE

// fix loading of startupScript
// добавлена поддержка пути с кириллическими символами
#define RB_SF

// поглощение совпадающих символов в строке при выборе из списка автозавершения
#define RB_ACMERGE

// восстановление сессии при открытии одиночного файла
#define RB_RSOL

// удаляет из обработки закомментированные строки и секции в props файлах
#define RB_PCMF

// добавляет команду IDM_NEXTFONTSET для переключения скриптом FontSet.lua набора шрифтов
#define RB_NFS

// фикс раскраски двоеточия для лексера Lua (пример: "obj:method1():method2()" )
#define RB_FLL

// изменение отображения пунктов меню Вкладки
#define RB_TMF

// добавляет иконку для окна параметров
#define RB_ICPW
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

