@echo off
rem ==============================================================
rem  Build html2img extension (PHP 8.2, VS 2022 x64)
rem  Adjust PHP_SDK if you move the SDK folder.
rem ==============================================================

rem ---- CONFIG ---------------------------------------------------
set "PHP_SDK=F:\development\steam\emulator_bot\php_html2img_ext\php-8.4.10-devel-vs17-x64"
set "EXT_DIR=%PHP_SDK%\src"
set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
rem ---------------------------------------------------------------

rem ---- Sanity checks -------------------------------------------
if not exist "%VSDEV%" (
    echo [ERROR] VSDevCmd not found: %VSDEV%
    goto :eof
)
if not exist "%PHP_SDK%\phpize.bat" (
    echo [ERROR] phpize.bat not found in %PHP_SDK%
    goto :eof
)
if not exist "%EXT_DIR%\config.w32" (
    echo [ERROR] %EXT_DIR%\config.w32 is missing
    goto :eof
)

rem ---- Enter extension dir & set MSVC env ----------------------
pushd "%EXT_DIR%"  || goto :eof
call "%VSDEV%" -arch=x64          || goto :eof

rem ---- Clean stale output --------------------------------------
if exist x64       rmdir /s /q x64
del /q Makefile config.nice.bat  2>nul

rem ---- phpize ---------------------------------------------------
echo Running phpize …
call "%PHP_SDK%\phpize.bat"       || goto :eof

rem  phpize ends in the SDK root; jump back to src
cd /d "%EXT_DIR%"                 || goto :eof

rem ---- configure (force JScript engine if needed) --------------
echo Configuring …
if exist configure.bat (
    call configure --enable-html2img %*
) else (
    cscript //nologo //E:jscript configure.js --enable-html2img %*
)
if errorlevel 1 goto :eof

rem ---- build ----------------------------------------------------
echo Building …
nmake /nologo                     || goto :eof

echo:
echo [OK] Built: %EXT_DIR%\x64\Release_TS\php_html2img.dll
popd
goto :eof
