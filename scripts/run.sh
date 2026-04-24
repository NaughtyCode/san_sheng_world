#!/bin/bash
export ANTHROPIC_API_KEY="${ANTHROPIC_API_KEY:-}"
./build/bin/Release/ai_agent "$@"
