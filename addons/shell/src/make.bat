@ECHO OFF
mingw32-make all -j8
if errorlevel 1 exit
mingw32-make clean