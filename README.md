# AI Agent

A minimal viable AI Agent based on the Anthropic API, supporting tool calling and Luau script extension.

## Requirements

- C++17 compiler
- CMake 3.16+
- Anthropic API key

## Quick Start

```bash
# Initialize dependencies
git submodule update --init --recursive

# Build
./scripts/build.sh

# Run
export ANTHROPIC_API_KEY="your-api-key"
./scripts/run.sh
```

## Project Structure

```
ai_agent/
├── src/
│   ├── main/       # Entry point
│   ├── common/     # Logger, Config, Utils
│   └── core/       # Agent loop, API client, tools, script engine
├── external/       # Third-party libraries
├── tests/          # Unit tests
├── scripts_luau/   # Lua script examples
└── scripts/        # Build/test/run scripts
```
