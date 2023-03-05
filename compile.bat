@echo off

rem execute 'vcvarsall x64' first
set DEBUG=1

if NOT EXIST build MD build
echo:
echo:


echo "compiling fscord.c ..."
set CC=clang
set CFLAGS=-std=c99 -c -Wall -D_CRT_SECURE_NO_WARNINGS
if %DEBUG%==1 set CFLAGS=%CFLAGS% -g
set IFLAGS=-I./libs/include -I./libs/include/libressl -Isrc
set SRC=./src/client/fscord.c
set DEST=./build/fscord.obj
%CC% %CFLAGS% %IFLAGS% -o %DEST% %SRC%
echo:
echo:


echo "compiling fscord_win32.cpp ..."
set CC=cl
set CFLAGS=/Wall /c /MD
if %DEBUG%==1 set CFLAGS=%CFLAGS% /Z7
set IFLAGS=/I./libs/include /I./libs/include/libressl /I./src /I./src/client
set SRC=./src/client/sys/win32/win32_fscord.cpp
set DEST=./build/win32_fscord.obj
%CC% %CFLAGS% %IFLAGS% /Fo"%DEST%" %SRC%
echo:
echo:


echo "linking ..."
set LFLAGS=User32.lib Gdi32.lib Ws2_32.lib Crypt32.lib Bcrypt.lib Advapi32.lib ./libs/static/libressl/crypto-50.lib ./libs/static/libressl/ssl-53.lib
if %DEBUG%==1 set LFLAGS=%LFLAGS% /DEBUG
link ./build/win32_fscord.obj ./build/fscord.obj %LFLAGS% /out:.\build\fscord.exe
