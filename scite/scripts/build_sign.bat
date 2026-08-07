:: build_sign.bat
:: Sign each architecture for all of Lexilla, Scintilla and SciTE for distribution
:: Called by build_all.bat

:: Requires enironment variables to be set
:: SIGN_TOOL: path to signtool.exe
:: SIGN_NAME: name of signer

"%SIGN_TOOL%" sign /n "%SIGN_NAME%" /t http://time.certum.pl /fd sha256 /v x64/scite/bin/SciTE.exe x64/scite/bin/Lexilla.dll x64/scite/bin/Scintilla.dll x64/scite/bin/Sc1.exe x86/scite/bin/SciTE.exe x86/scite/bin/Lexilla.dll x86/scite/bin/Scintilla.dll x86/scite/bin/Sc1.exe ARM64/scite/bin/SciTE.exe ARM64/scite/bin/Lexilla.dll ARM64/scite/bin/Scintilla.dll ARM64/scite/bin/Sc1.exe

REM Can verify like this:
REM ~ "%SIGNTOOL%" verify /pa x64/scite/bin/SciTE.exe
REM Or verbose:
REM ~ "%SIGNTOOL%" verify /v /pa x64/scite/bin/SciTE.exe
