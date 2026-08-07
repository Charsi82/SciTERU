:: build_all.bat Run all the release building phases

:: Set environment variables used by other batch files, must be customized to suit installation
:: REPOSITORY_DIRECTORY: path to source code repositories, parent of scite, lexilla, and scintilla directories
:: MSVC_DIRECTORY: path to MSVC setup batch files like vcvars64.bat
:: SIGN_TOOL: path to signtool.exe
:: SIGN_NAME: name of signer

set REPOSITORY_DIRECTORY=..\hg
rem Find base of latest Visual Studio with
rem "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath
set "MSVC_DIRECTORY=G:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build"
rem Only the 32-bit signtool works for me
set "SIGN_TOOL=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\signtool.exe"
set "SIGN_NAME=Neil Hodgson"

call build_source
call build_compile
call build_sign
call build_archive
