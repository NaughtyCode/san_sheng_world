@echo off
setlocal

set "ANTHROPIC_API_KEY=%ANTHROPIC_API_KEY%"
if "%ANTHROPIC_API_KEY%"=="" (
    echo WARNING: ANTHROPIC_API_KEY not set
)

"%~dp0..\build\bin\Release\ai_agent.exe" %*
