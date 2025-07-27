@echo off
REM Build html2img using Visual Studio 17 (2022)
REM Environment variables:
REM   PHP_PREFIX - prefix where phpize.bat resides (optional)
REM   VSDEV      - path to VsDevCmd.bat (optional)
setlocal ENABLEDELAYEDEXPANSION
set "LITEHTML_DIR=3rdparty\litehtml"
if not exist "%LITEHTML_DIR%" (
    git clone --depth=1 --branch 35ecd69d05e72b0148204a576db62c2148084193 https://github.com/litehtml/litehtml.git "%LITEHTML_DIR%"
    if errorlevel 1 exit /b 1
    pushd "%LITEHTML_DIR%"
    git submodule update --init --recursive
    patch -p1 < ..\..\patches\litehtml-static.patch
    popd
)
if "%VSDEV%"=="" (
    set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
)
call "%VSDEV%" -arch=x64
if errorlevel 1 exit /b 1
if not "%PHP_PREFIX%"=="" (
    set "PHPIZE=%PHP_PREFIX%\phpize.bat"
) else (
    set "PHPIZE=phpize"
)
%PHPIZE%
if errorlevel 1 exit /b 1
configure --enable-html2img %*
if errorlevel 1 exit /b 1
nmake /nologo
if errorlevel 1 exit /b 1
endlocal
