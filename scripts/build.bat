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

:: Detect OpenSSL for HTTPS support
set OPENSSL_ROOT_DIR=%OPENSSL_ROOT_DIR%
if "%OPENSSL_ROOT_DIR%"=="" (
    for %%d in (
        "C:\Users\user\anaconda3\Library"
        "C:\Program Files\OpenSSL-Win64"
        "C:\Program Files\OpenSSL"
        "C:\vcpkg\packages\openssl_x64-windows"
    ) do (
        if exist "%%~d\include\openssl\ssl.h" (
            if not defined OPENSSL_ROOT_DIR set "OPENSSL_ROOT_DIR=%%~d"
        )
    )
)
if not "%OPENSSL_ROOT_DIR%"=="" (
    echo Using OpenSSL: %OPENSSL_ROOT_DIR%
) else (
    echo WARNING: OpenSSL not found, HTTPS support may be disabled
)

if not exist build mkdir build
cd build

cmake .. -DBUILD_TESTS=ON -DOPENSSL_ROOT_DIR="%OPENSSL_ROOT_DIR%"
if errorlevel 1 goto :error

cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 goto :error

echo Build complete.
exit /b 0

:error
echo Build failed!
exit /b 1
