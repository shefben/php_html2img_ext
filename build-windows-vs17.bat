@echo off
rem ─────────────────────────────────────────────────────────────────────────────
rem  build_html2img.bat  –  PHP‑8.2 html2img extension (VS‑2022 x64)
rem  Folder layout (after your move):
rem      %PHP_SDK%                    = …\php-8.2.29-devel-vs16-x64
rem      %EXT_ROOT% (= %PHP_SDK%\src) = …\php-8.2.29-devel-vs16-x64\src
rem ─────────────────────────────────────────────────────────────────────────────

setlocal ENABLEDELAYEDEXPANSION

rem — Adjust these three only if you move things again ————————————————
set "PHP_SDK=F:\development\steam\emulator_bot\php_html2img_ext\php-8.2.29-devel-vs16-x64"
set "EXT_ROOT=%PHP_SDK%\src"
set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

rem — Sanity checks ————————————————————————————————————————————————
if not exist "%VSDEV%" (
    echo [ERROR] VSDevCmd not found: %VSDEV%
    exit /b 1
)
if not exist "%PHP_SDK%\phpize.bat" (
    echo [ERROR] phpize.bat missing under "%PHP_SDK%"
    exit /b 1
)
if not exist "%EXT_ROOT%\config.w32" (
    echo [ERROR] config.w32 not found in "%EXT_ROOT%"
    exit /b 1
)

rem — Enter extension root (where config.w32 & cpp files live) ————————
pushd "%EXT_ROOT%" || (echo [ERROR] cd failure & exit /b 1)

rem — MSVC toolchain —————————————————————————————————————————————
call "%VSDEV%" -arch=x64 || exit /b 1

rem — Bootstrapping (phpize) ————————————————————————————————
echo Running phpize …
call "%PHP_SDK%\phpize.bat" || exit /b 1

rem  phpize leaves you in %PHP_SDK%; hop right back to src
cd /d "%EXT_ROOT%" || (echo [ERROR] lost path after phpize & exit /b 1)

rem — Configure & build ————————————————————————————————————————
echo Configuring …
call configure --enable-html2img %* || exit /b 1

echo Building …
nmake /nologo || exit /b 1

popd
echo.
echo ✔ html2img built — look in %EXT_ROOT%\Release_TS (or Debug_TS) for html2img.dll
endlocal
