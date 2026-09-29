@ECHO OFF
mingw32-make all
if errorlevel 1 exit
mingw32-make clean