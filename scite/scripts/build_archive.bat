:: build_archive.bat
:: Archive each architecture into release directory
:: Called by build_all.bat

:: Requires hg, git and zip to be in the path. nmake, cl, and link are found by vcvars*.bat

:: Discover the SciTE version as that is used in file and directory names, override on command line
set SCITE_VERSION=%1
if [%1] == [] for /F %%i IN (scite\version.txt) do set "SCITE_VERSION=%%i"
set "UPLOAD_DIRECTORY=upload%SCITE_VERSION%"

for /F %%i IN (scite\src\lexillaVersion.txt) do set "LEXILLA_WANTED=%%i"
for /F %%i IN (scite\src\scintillaVersion.txt) do set "SCINTILLA_WANTED=%%i"

rd /s/q %UPLOAD_DIRECTORY%

:: Copy source archives into correctly numbered upload directory
echo %UPLOAD_DIRECTORY%
mkdir %UPLOAD_DIRECTORY%
copy lexilla.zip %UPLOAD_DIRECTORY%\lexilla%LEXILLA_WANTED%.zip
copy scintilla.zip %UPLOAD_DIRECTORY%\scintilla%SCINTILLA_WANTED%.zip
copy scite.zip %UPLOAD_DIRECTORY%\scite%SCITE_VERSION%.zip

echo Lexilla wanted = %LEXILLA_WANTED%
echo Scintilla wanted = %SCINTILLA_WANTED%

:: Archive the 64-bit executables

pushd x64\scite
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

copy x64\wscite.zip %UPLOAD_DIRECTORY%\wscite%SCITE_VERSION%.zip
copy x64\Sc1.exe %UPLOAD_DIRECTORY%\Sc%SCITE_VERSION%.exe

:: Archive the 32-bit executables

pushd x86\scite
move bin\SciTE.exe bin\SciTE32.exe
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

copy x86\wscite.zip %UPLOAD_DIRECTORY%\wscite32_%SCITE_VERSION%.zip
copy x86\Sc1.exe %UPLOAD_DIRECTORY%\Sc32_%SCITE_VERSION%.exe

:: Archive the ARM64 executables

pushd ARM64\scite
move bin\SciTE.exe bin\SciTEArm.exe
copy bin\Sc1.exe ..\Sc1.exe
call zipwscite
popd

copy ARM64\wscite.zip %UPLOAD_DIRECTORY%\wsciteArm_%SCITE_VERSION%.zip
copy ARM64\Sc1.exe %UPLOAD_DIRECTORY%\ScArm_%SCITE_VERSION%.exe

:: directories remain so can be examined if something went wrong
