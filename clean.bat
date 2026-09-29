echo off
chcp 1251 > nul
del /q /s *.bak *.log *.pdb *.bsc *.obj *.iobj *.ipdb *.tlog *.recipe *.idb *.ilk *.lastbuildstate *.exp *.aps
setlocal
set fpath=%~dps0

rem remove intermediate
set rdir=intermediate
call :func %fpath:~0,-1%

rem remove .vs
set rdir=.vs
call :func %fpath:~0,-1%

goto end
:func
for /f "delims=" %%i in ('dir %1 /a:d /b') do IF /I %%i==%rdir% ( rmdir /s /q %1\%%i && echo deleted %1\%%i ) ELSE ( call :func %1\%%i )
exit /b
:end

del /s /q addons\*.dll addons\*.a lexilla\*liblexilla.a scite\*scite.exe
echo done.