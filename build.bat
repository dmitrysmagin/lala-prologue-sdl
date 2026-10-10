@echo off
REM lala-sdl Windows fallback build (cmd.exe, no make/pkg-config shell needed)
REM Mirrors Makefile: local .tmp temp redirect + gcc + SDL2 via pkg-config if present.
mkdir .tmp 2>nul
set TMPDIR=%CD%\.tmp
set TEMP=%CD%\.tmp
set TMP=%CD%\.tmp
set GCC_TMPDIR=%CD%\.tmp
mkdir build 2>nul
mkdir dist 2>nul

where pkg-config >nul 2>nul
if %ERRORLEVEL%==0 (
  for /f "delims=" %%i in ('pkg-config --cflags sdl2') do set SDL_CFLAGS=%%i
  for /f "delims=" %%i in ('pkg-config --libs sdl2') do set SDL_LIBS=%%i
) else (
  set SDL_CFLAGS=-IC:/msys64/ucrt64/include/SDL2 -Dmain=SDL_main
  set SDL_LIBS=-LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -mconsole
)

setlocal enabledelayedexpansion
set OBJS=
for %%f in (src\*.c) do (
  gcc -std=c99 -Wall -Wextra -O2 %SDL_CFLAGS% -c %%f -o build\%%~nf.o
  if errorlevel 1 exit /b 1
  set OBJS=!OBJS! build\%%~nf.o
)
gcc !OBJS! -o dist\lala.exe %SDL_LIBS%
if errorlevel 1 exit /b 1
echo Build OK: dist\lala.exe
