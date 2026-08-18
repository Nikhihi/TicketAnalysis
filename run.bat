@echo off
setlocal
cd /d %~dp0

call "%~dp0build.bat"
if errorlevel 1 exit /b 1

echo.
echo Running analysis...
"%~dp0build\Release\ticket_analysis.exe"
if errorlevel 1 exit /b 1

echo.
echo Dashboard generated: output\dashboard.html
echo Open it in a browser to view the interactive report.
endlocal
