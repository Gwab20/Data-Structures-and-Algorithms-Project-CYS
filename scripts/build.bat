@echo off
echo ================================================
echo Building Anti-Aircraft Radar Project - Phase 1
echo ================================================

if not exist ..\build mkdir ..\build

echo Compiling source files...

:: Only compile the files that currently exist
g++ -std=c++11 -I..\include ^
    ..\src\utils\MathUtils.cpp ^
    ..\src\radar\Radar.cpp ^
    ..\src\radar\Target.cpp ^
    ..\src\radar\Gun.cpp ^
    ..\src\main.cpp ^
    -o ..\build\radar_sim.exe

if %errorlevel% equ 0 (
    echo ✅ Build successful!
    echo.
    echo Running simulation...
    echo ================================================
    ..\build\radar_sim.exe
) else (
    echo ❌ Build failed!
    pause
    exit /b 1
)