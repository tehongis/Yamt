@echo off
REM Build script for Yamt injector — run from __Developer Command Prompt for VS 2026__

setlocal
set SRC_DIR=%~dp0\src\
set OUT_DIR=%SRC_DIR%..\build\

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

cl /nologo /std:c++20 /EHsc /O2 /W4 /DUNICODE /D_UNICODE "%SRC_DIR%main.cpp" /link /OUT:"%OUT_DIR%injector.exe" comctl32.lib user32.lib gdi32.lib
if %ERRORLEVEL% neq 0 (
  echo Build failed with errorlevel %ERRORLEVEL%.
  exit /b %ERRORLEVEL%
)

echo Build succeeded: "%OUT_DIR%injector.exe"
endlocal