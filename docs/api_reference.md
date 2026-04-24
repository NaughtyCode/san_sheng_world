# API Reference

## Common Layer

### Logger (`src/common/logger.hpp`)

- `Logger::instance()` — get singleton
- `log(LogLevel, fmt, ...)` — log message with level
- `debug/info/warning/error(fmt, ...)` — convenience logging
- `set_level(LogLevel)` — set minimum log level
- `set_file(path)` — redirect output to file
- LogLevel: Debug, Info, Warning, Error

### Config (`src/common/config.hpp`)

- `load_from_file(path)` — load JSON config file
- `load_from_env()` — read ANTHROPIC_API_KEY, ANTHROPIC_MODEL, LOG_LEVEL, LOG_FILE, ANTHROPIC_BASE_URL
- `get<T>(key, default)` — typed getter
- `set(key, value)` — set string value
- `has(key)` — check if key exists

### Utils (`src/common/utils.hpp`)

- `trim(s)` — remove leading/trailing whitespace
- `now()` — ISO 8601 timestamp string
- `to_lower(s)` — lowercase string
- `starts_with(s, prefix)` / `ends_with(s, suffix)`

## API Client Layer

### HttpClient (`src/core/api/http_client.hpp`)

- `set_base_url(url)` — set base URL (include scheme, e.g. `https://api.anthropic.com`)
- `set_header(key, value)` — add default header
- `set_timeout(seconds)` — request timeout
- `post(path, body)` -> `HttpResponse` — POST JSON
- `get(path)` -> `HttpResponse` — GET request

### AnthropicClient (`src/core/api/anthropic_client.hpp`)

- `set_api_key(key)` — set x-api-key header
- `set_model(model)` — model name (default: claude-sonnet-4-6)
- `set_base_url(url)` — API base URL
- `messages_create(model, messages, tools, max_tokens)` -> `json` — Messages API call

### Message struct

- `role` — "user", "assistant", "tool"
- `content` — text content
- `tool_call_id`, `tool_name`, `tool_input` — for tool_use messages

### ToolDefinition struct

- `name`, `description`, `input_schema` (JSON Schema)

## Agent Core

### Conversation (`src/core/conversation.hpp`)

- `add_user_msg(content)` — add user message
- `add_assistant_msg(content, tool_name, tool_id, tool_input)` — add assistant response
- `add_tool_result(tool_call_id, result)` — add tool result
- `get_messages()` -> `vector<Message>` — full message history
- `clear()` — reset conversation

### ToolRegistry (`src/core/tool_registry.hpp`)

- `register(name, description, input_schema, handler)` — register tool
- `call(name, args)` -> `string` — invoke tool handler
- `has(name)` -> `bool` — check registration
- `get_definitions()` -> `vector<ToolDefinition>` — tool list for API

### AgentLoop (`src/core/agent_loop.hpp`)

- `set_api_key(key)` / `set_model(model)` — API configuration
- `set_max_tokens(n)` / `set_max_iterations(n)` — loop limits
- `register_tool(name, desc, schema, handler)` — add tool
- `load_builtin_tools()` — register calculator and script tools
- `load_script_tool(path, func_name)` — load Luau script as tool
- `run(user_input)` -> `string` — execute agent loop
- `script_engine()` -> `LuauEngine&` — access script engine

## Script Engine

### LuauEngine (`src/core/script/luau_engine.hpp`)

- `init()` — create sandboxed Lua state
- `load_script(path)` — load .luau file
- `load_string(name, source)` — compile and execute Lua code
- `call_function(name, args)` -> `json` — call Lua function
- `register_cpp_function(name, handler)` — expose C++ function to Lua

### ScriptTool (`src/core/script/script_tool.hpp`)

- Adapter between ToolRegistry and LuauEngine
- Wraps a named Luau function as a tool handler
