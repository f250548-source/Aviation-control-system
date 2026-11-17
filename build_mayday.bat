@echo off
REM Build MayDay-404 with the Visual Studio developer compiler (cl)
REM Run from "x64 Native Tools Command Prompt for VS" OR after vcvars64.bat

set SRC=main.cpp utils.cpp menu.cpp radar_mst.cpp game_engine.cpp
set OUT=mayday404.exe

echo Compiling MayDay-404...
cl /EHsc /W3 /Fe:%OUT% %SRC%
if %ERRORLEVEL% NEQ 0 (
  echo Build failed. Open "x64 Native Tools Command Prompt for VS" and rerun.
  exit /b 1
)
echo Build OK: %OUT%
