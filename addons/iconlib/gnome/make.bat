@ECHO OFF

CD /D "%~dp0"
windres -o resfile.o toolbar.rc
IF ERRORLEVEL 1 EXIT

ld --strip-all --dll -o ../../bin\gnome.dll resfile.o
IF ERRORLEVEL 1 EXIT

DEL resfile.o