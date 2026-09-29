@ECHO OFF
mingw32-make a -j8
mingw32-make lua -j8
if errorlevel 1 exit

move /Y liblua.a "../../bin"
move /Y lua.exe "../../bin"
if errorlevel 1 exit

mingw32-make clean