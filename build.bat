@ECHO OFF
SET root=%~dp0
MODE CON COLS=120 LINES=2000
chcp 1251 > nul
REM -----------------------------------------------------
set clang=1
REM -----------------------------------------------------

if defined clang (
	CALL :if_exist "clang.exe"
) else (
	CALL :if_exist "gcc.exe"
)

IF ERRORLEVEL 1 (
	ECHO Error : Please install MinGW!
	ECHO - For more information visit: https://www.mingw-w64.org/getting-started/msys2/
	GOTO error
)

IF "%1"=="clear" CALL :clear

rem -----------------------------------------------------
SET icons_dir=%root%pack\toolbar\
CALL :header Make GNOME
CALL %root%addons\iconlib\gnome\make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\gnome.dll %icons_dir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage GNOME

rem -----------------------------------------------------
CALL :header Make COOL
CALL %root%addons\iconlib\cool\make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\cool.dll %icons_dir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage COOL

rem -----------------------------------------------------
CALL :header Make libUTF
CD %root%addons\utf\src
CALL make
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage libUTF

rem -----------------------------------------------------
CALL :header Make libLua
CD %root%addons\lua_src\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\lua.exe %root%pack\utils\
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\luac.exe %root%pack\utils\
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage libLua

rem -----------------------------------------------------
SET libsdir=%root%pack\tools\LuaLib\
rem -----------------------------------------------------
CALL :header Make LFS
CD %root%addons\lfs\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\lfs.dll %libsdir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage LFS

REM rem -----------------------------------------------------
CALL :header Make LPEG
CD %root%addons\lpeg\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\lpeg.dll %libsdir%
IF ERRORLEVEL 1 GOTO error
copy /Y %root%addons\lpeg\re.lua %libsdir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage LPEG

REM rem -----------------------------------------------------
CALL :header Make GUI
CD %root%addons\gui\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\gui.dll %libsdir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage GUI

rem -----------------------------------------------------
CALL :header Make WINREG
CD %root%addons\winreg\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\winreg.dll %libsdir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage WINREG

rem -----------------------------------------------------
CALL :header Make EVENTS
CD %root%addons\events\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\events.dll %libsdir%
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage EVENTS

rem -----------------------------------------------------
CALL :header Make SHELL
CD %root%addons\shell\src
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%addons\bin\shell.dll %libsdir%
CALL :completed_stage SHELL

rem -----------------------------------------------------
CALL :header Make Lexilla
CD %root%lexilla
CALL make
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage Lexilla

rem -----------------------------------------------------
CALL :header Make SCITE
CD %root%scite
CALL make
IF ERRORLEVEL 1 GOTO error
move /Y %root%scite\bin\scite.exe %root%pack\
IF ERRORLEVEL 1 GOTO error
CALL :completed_stage SciTE

rem -----------------------------------------------------
call :clear

rem -----------------------------------------------------
GOTO completed

:completed_stage
ECHO Building %* successfully completed!
ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GOTO :EOF

:completed
ECHO.
ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO Building SciTE-Ru successfully completed!
TITLE SciTE-Ru completed
GOTO end

:error
ECHO.
ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO Errors were found!
GOTO end

:error_install
ECHO.
ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO Please install MinGW!
GOTO end

:if_exist
FOR /f %%i IN (%1) DO IF "%%~$PATH:i"=="" EXIT /b 1
EXIT /b 0

:header
ECHO.
ECHO ^> ~~~~~~~ [ %* ] ~~~~~~~
TITLE Create SciTE-Ru: %*
GOTO :EOF

:clear
CD %root%
DEL /Q lexilla\bin\*.a > NUL
CD addons
DEL /S /Q *.a *.aps *.bsc *.dll *.dsw *.exe *.idb *.ilc *.ild *.ilf *.ilk *.ils *.lib *.map *.ncb *.obj *.o *.opt *.pdb *.plg *.res *.sbr *.tds *.exp > NUL 2<&1
GOTO :EOF

:end
CD %root%
