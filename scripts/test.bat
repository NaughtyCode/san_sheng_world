@echo off
setlocal

call "%~dp0build.bat"
if errorlevel 1 exit /b 1

cd "%~dp0..\build"
set BUILD_TYPE=%BUILD_TYPE%
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

ctest --output-on-failure -C %BUILD_TYPE%
if errorlevel 1 (
    echo Some tests failed!
    exit /b 1
)
echo All tests passed.
