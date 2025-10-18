@echo off
echo Building Anti-Aircraft Radar Project...

if not exist ..\build mkdir ..\build

g++ -std=c++11 -I..\include ^
    ..\src\utils\MathUtils.cpp ^
    ..\src\radar\Target.cpp ^
    ..\src\radar\Gun.cpp ^
    ..\src\radar\Radar.cpp ^
    ..\src\main.cpp ^
    -o ..\build\radar_sim.exe

if %errorlevel% equ 0 (
    echo Build successful! Running simulation...
    echo.
    ..\build\radar_sim.exe
) else (
    echo Build failed!
    exit /b 1
)