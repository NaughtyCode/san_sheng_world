@echo off
setlocal enabledelayedexpansion

set BUILD_TYPE=%BUILD_TYPE%
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

:: Find and run VS Dev Cmd
set "VSDEVCMD="
for /f "delims=" %%i in ('dir /s /b "D:\Program Files\Microsoft Visual Studio\2022\*VsDevCmd.bat" 2^>nul') do (
    if not defined VSDEVCMD set "VSDEVCMD=%%i"
)
if defined VSDEVCMD (
    echo Using: %VSDEVCMD%
    call "%VSDEVCMD%"
) else (
    echo WARNING: Visual Studio not found, trying default compiler
)

if not exist build mkdir build
cd build

cmake .. -DBUILD_TESTS=ON
if errorlevel 1 goto :error

cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 goto :error

echo Build complete.
exit /b 0

:error
echo Build failed!
exit /b 1
