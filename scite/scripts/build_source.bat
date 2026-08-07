:: build_source.bat Archive a version of SciTE into lexilla, scintilla, and scite subdirectories.
:: Zip into source archives lexilla.zip, scintilla.zip, and scite.zip
:: This batch file is distributed inside scite but is commonly copied out into its own working directory
:: Called by build_all.bat

:: Requires enironment variables to be set
:: REPOSITORY_DIRECTORY: path to source code repositories, parent of scite, lexilla, and scintilla directories

:: Requires hg, git and zip to be in the path

rd /s/q lexilla scintilla scite
del/q lexilla.zip scintilla.zip scite.zip

:: Discover the SciTE version as that is used in file and directory names, override on command line
set SCITE_VERSION=%1
if [%1] == [] for /F %%i IN (%REPOSITORY_DIRECTORY%\scite\version.txt) do set "SCITE_VERSION=%%i"
set SCITE_TAG=rel-%SCITE_VERSION:~0,-2%-%SCITE_VERSION:~-2,1%-%SCITE_VERSION:~-1%

echo SciTE wanted = %SCITE_VERSION% %SCITE_TAG%

hg archive -R %REPOSITORY_DIRECTORY%/scite --rev %SCITE_TAG% scite

:: Find the Lexilla and Scintilla versions corresponding to the SciTE version

for /F %%i IN (scite\src\lexillaVersion.txt) do set "LEXILLA_WANTED=%%i"
for /F %%i IN (scite\src\scintillaVersion.txt) do set "SCINTILLA_WANTED=%%i"

if [%LEXILLA_WANTED%] == [] (set LEXILLA_TAG=) else set LEXILLA_TAG=rel-%LEXILLA_WANTED:~0,-2%-%LEXILLA_WANTED:~-2,1%-%LEXILLA_WANTED:~-1%
set SCINTILLA_TAG=rel-%SCINTILLA_WANTED:~0,-2%-%SCINTILLA_WANTED:~-2,1%-%SCINTILLA_WANTED:~-1%

echo Lexilla wanted = %LEXILLA_WANTED% %LEXILLA_TAG%
echo Scintilla wanted = %SCINTILLA_WANTED% %SCINTILLA_TAG%

if [%LEXILLA_TAG%] == [] (
git clone %REPOSITORY_DIRECTORY%/lexilla lexilla
) else (
git -c advice.detachedHead=false clone --branch %LEXILLA_TAG% %REPOSITORY_DIRECTORY%/lexilla lexilla
)

hg archive -R %REPOSITORY_DIRECTORY%/scintilla --rev %SCINTILLA_TAG% scintilla

:: Create source archives
pushd lexilla
call zipsrc
popd
hg archive -R %REPOSITORY_DIRECTORY%/scintilla --rev %SCINTILLA_TAG% scintilla.zip
pushd scite
call zipsrc
popd

:: directories remain so can be examined if something went wrong