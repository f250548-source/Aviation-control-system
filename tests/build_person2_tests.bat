@echo off
REM Build and run the Person 2 test runner (Modules 4-8).
REM Run from "x64 Native Tools Command Prompt for VS" OR after vcvars64.bat.
REM This is a separate executable; it does not touch the game build.
REM Debug CRT (/MDd /D_DEBUG) enables the per-group memory-leak checks.

cd /d "%~dp0.."
if not exist tests\obj mkdir tests\obj

set SRC=tests\person2_tests.cpp bst_flight.cpp avl_flight.cpp space_graph.cpp radar_sweep.cpp route_finder.cpp radar_mst.cpp

echo Compiling Person 2 tests...
cl /nologo /EHsc /W3 /MDd /D_DEBUG /Fo:tests\obj\ /Fe:tests\person2_tests.exe %SRC%
if %ERRORLEVEL% NEQ 0 (
  echo Test build failed.
  exit /b 1
)

REM Run from the repository root so the real airspace.txt is found (read-only)
tests\person2_tests.exe
exit /b %ERRORLEVEL%
