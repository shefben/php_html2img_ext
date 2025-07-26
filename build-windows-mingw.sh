@echo off
REM Cross-build html2img using MinGW-w64
REM Environment variables:
REM   PHP_PREFIX   - prefix with phpize.bat if not in PATH
REM   GD_LIB       - path to gd.lib (optional)
REM   FREETYPE_LIB - path to freetype.lib (optional)
REM   PNG_LIB      - path to png.lib (optional)
REM   JPEG_LIB     - path to jpeg.lib (optional)
setlocal ENABLEDELAYEDEXPANSION
set CC=x86_64-w64-mingw32-gcc
set CXX=x86_64-w64-mingw32-g++
if not "%PHP_PREFIX%"=="" (
    set PHPIZE=%PHP_PREFIX%\phpize
) else (
    set PHPIZE=phpize
)
%PHPIZE%
./configure --host=x86_64-w64-mingw32 --enable-html2img ^
    CPPFLAGS="-static -static-libstdc++ -static-libgcc"
if errorlevel 1 exit /b 1
make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b 1
endlocal
