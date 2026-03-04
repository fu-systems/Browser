@echo off
REM Pane Browser — Windows Build Script (MSVC)
REM
REM Prerequisites:
REM   1. Visual Studio 2022 (or Build Tools) with C desktop workload
REM   2. CMake 3.20+ (included with VS)
REM   3. (Optional) vcpkg with freetype for font rendering
REM
REM Run from "x64 Native Tools Command Prompt for VS 2022"

echo.
echo  Pane Browser v0.1.0 — Windows Build
echo  ====================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found. Install CMake or use Visual Studio Developer Prompt.
    exit /b 1
)

REM Navigate to source root (parent of dist/)
cd /d "%~dp0..\.."

REM Create build directory
if not exist build-win mkdir build-win
cd build-win

REM Configure
echo Configuring with CMake...
if defined VCPKG_ROOT (
    echo Using vcpkg at %VCPKG_ROOT%
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
) else (
    cmake .. -G "Visual Studio 17 2022" -A x64
)

if errorlevel 1 (
    echo.
    echo CMake configuration failed.
    exit /b 1
)

REM Build
echo.
echo Building Release...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

echo.
echo  Build successful!
echo  Binaries are in: build-win\Release\
echo.
echo  pane.exe     — GUI Browser
echo  pane_cli.exe — CLI Demo
echo.

REM Copy to dist
if not exist "..\dist\pane-windows-x86_64" mkdir "..\dist\pane-windows-x86_64"
copy /y Release\pane.exe ..\dist\pane-windows-x86_64\ >nul 2>&1
copy /y Release\pane_cli.exe ..\dist\pane-windows-x86_64\ >nul 2>&1

echo Done.
