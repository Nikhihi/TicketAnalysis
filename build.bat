@echo off
setlocal
cd /d %~dp0

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1

cmake --build build --config Release
if errorlevel 1 exit /b 1

echo.
echo Build OK: build\Release\ticket_analysis.exe
endlocal
