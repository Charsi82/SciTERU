:: build_compile.bat Compile Lexilla, Scintilla, and SciTE for x64, x86, and ARM64
:: Save results into a SciTE subdirectory inside x64, x86, and ARM64 directories
:: Called by build_all.bat

:: Requires enironment variables to be set
:: MSVC_DIRECTORY: path to MSVC setup batch files like vcvars64.bat

:: Requires source to already be in lexilla, scintilla, and scite subdirectories.

:: Running after Visual C++ set up with vcvars64 or vcvars32 may cause confusing failures
IF DEFINED VisualStudioVersion (ECHO VisualStudio is active && exit)

rd /s/q x64 x86 ARM64

:: Build the 64-bit executables

setlocal
call "%MSVC_DIRECTORY%\vcvars64.bat"

pushd lexilla\src
nmake -f lexilla.mak
popd

pushd scintilla
pushd win32
nmake -f scintilla.mak
popd
del/q bin\*.pdb
popd

pushd scite\win32
nmake -f scite.mak
popd

:: Xcopy scite into x64 directory
xcopy /I /S scite x64\scite

:: Clean all
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd
pushd lexilla
call delbin
popd

endlocal

:: Build the 32-bit executables

setlocal
call "%MSVC_DIRECTORY%\vcvars32.bat"

pushd lexilla\src
nmake -f lexilla.mak
popd

pushd scintilla
pushd win32
nmake -f scintilla.mak
popd
del/q bin\*.pdb
popd

pushd scite\win32
nmake -f scite.mak
popd

:: Xcopy scite into x86 directory
xcopy /I /S scite x86\scite

:: Clean all
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd
pushd lexilla
call delbin
popd

endlocal

:: Build the ARM64 executables

setlocal
call "%MSVC_DIRECTORY%\vcvarsamd64_arm64.bat"

pushd lexilla\src
nmake -f lexilla.mak
popd

pushd scintilla
pushd win32
nmake -f scintilla.mak
popd
del/q bin\*.pdb
popd

pushd scite\win32
nmake -f scite.mak ARM64=1
popd

:: Xcopy scite into x86 directory
xcopy /I /S scite ARM64\scite

:: Clean all
pushd scite
call delbin
popd
pushd scintilla
call delbin
popd
pushd lexilla
call delbin
popd

endlocal

:: all directories remain so can be examined if something went wrong
