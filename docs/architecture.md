# Architecture

## Overview

AI Agent based on Anthropic Messages API with tool calling and Luau script extension. Built with C++17 and CMake.

## Layers

```
┌─────────────────────────────────────────────────┐
│              Entry Layer (src/main)               │
│  main.cpp: config init, agent setup, CLI loop     │
├─────────────────────────────────────────────────┤
│            Agent Core Layer (src/core)             │
│  AgentLoop: request-response-tool loop            │
│  Conversation: message history management         │
│  ToolRegistry: tool registration and invocation   │
├──────────────────────────┬──────────────────────┤
│  API Client Layer         │  Script Engine       │
│  HttpClient (cpp-httplib) │  LuauEngine (Luau)   │
│  AnthropicClient          │  ScriptTool adapter  │
├──────────────────────────┴──────────────────────┤
│           Common Layer (src/common)               │
│  Logger: leveled logging to console/file          │
│  Config: JSON + environment variable config       │
│  Utils: string/time helpers                       │
├─────────────────────────────────────────────────┤
│          Third-Party Dependencies                 │
│  cpp-httplib | nlohmann/json | Luau              │
└─────────────────────────────────────────────────┘
```

## Data Flow

1. User input -> AgentLoop.run()
2. AgentLoop adds user message to Conversation
3. AgentLoop sends Messages API request via AnthropicClient
4. AnthropicClient constructs JSON request and POSTs via HttpClient
5. Response parsed: if tool_use, AgentLoop calls ToolRegistry
6. Tool result added to Conversation as tool_result message
7. Loop continues until model returns plain text or max_iterations reached

## Key Design Decisions

- **Static library per module**: agent_common and agent_core are static libraries
- **Singleton Logger**: global access via Logger::instance()
- **JSON everywhere**: nlohmann::json used for tool definitions, API payloads, and Luau interop
- **Sandboxed Luau**: dangerous globals (os, io, debug, loadfile, dofile, require) removed
- **Manual sandbox**: luaL_sandbox not used (makes globals read-only), instead individual globals removed
