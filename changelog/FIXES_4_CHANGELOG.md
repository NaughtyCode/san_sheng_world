# Fixes Issue #4 — 修改记录

## 问题

API 请求返回 400 错误，错误信息提示 tools 数组缺少 `type` 字段：
```
[ERROR] AnthropicClient: API error 400 from https://api.deepseek.com/chat/completions:
{"error":{"message":"Failed to deserialize the JSON body into the target type:
tools[0]: missing field `type` at line 1 column 339",...}}
[ERROR] AgentLoop: empty response from API
Error: failed to get response from API
```

## 根因分析

代码使用 **Anthropic Messages API 格式** 构造请求体，但端点 `https://api.deepseek.com/chat/completions` 是 **OpenAI Chat Completions API** 兼容端点，两者格式不兼容：

| 差异项 | Anthropic 格式（修复前） | OpenAI 格式（修复后） |
|--------|--------------------------|------------------------|
| 工具定义 | `{name, description, input_schema}` | `{type: "function", function: {name, description, parameters}}` |
| assistant 工具调用 | content 数组含 `{type: "tool_use", id, name, input}` | `tool_calls: [{id, type: "function", function: {name, arguments}}]` |
| tool 结果消息 | content 数组含 `{type: "tool_result", tool_use_id, content}` | `role: "tool", tool_call_id, content` (纯字符串) |
| 响应文本 | `content[{type: "text", text: "..."}]` | `choices[0].message.content` (纯字符串) |
| 停止判定 | `stop_reason: "end_turn"` | `choices[0].finish_reason: "stop"` |
| 工具调用判定 | `content` 数组中存在 `type: "tool_use"` 块 | `message.tool_calls` 数组非空 |

## 修改内容

### 1. `src/common/constants.hpp` — 新增 OpenAI 格式常量声明

新增以下 JSON 键与值常量用于 OpenAI API 格式：
- `JSON_FUNCTION` / `VALUE_FUNCTION` — tool_calls 中 type 值 `"function"`
- `JSON_PARAMETERS` — OpenAI 工具参数键 `"parameters"`（对应 Anthropic 的 `input_schema`）
- `JSON_TOOL_CALLS` / `VALUE_TOOL_CALLS` — assistant 消息工具调用数组及 finish_reason 值
- `JSON_ARGUMENTS` — tool_calls 中 JSON 字符串格式的参数
- `JSON_CHOICES` / `JSON_MESSAGE` / `JSON_FINISH_REASON` — OpenAI 响应结构
- `JSON_DELTA` / `JSON_INDEX` — 流式响应字段（预留）
- `VALUE_STOP` — finish_reason: `"stop"`

### 2. `src/common/constants.cpp` — 新增 OpenAI 格式常量定义

对应声明添加所有新常量的定义。

### 3. `src/core/api/anthropic_client.cpp` — 请求体格式迁移

| 函数 | 改动 |
|------|------|
| `build_request_body()` | 工具数组从 Anthropic 格式 `{name, description, input_schema}` 改为 OpenAI 格式 `{type: "function", function: {name, description, parameters}}` |
| `messages_to_api_format()` | 消息格式全面迁移：assistant 工具调用从 content 块数组改为 `tool_calls` 数组；tool 结果从 content 块数组改为字符串 content + `tool_call_id`；纯文本消息 content 从数组改为字符串 |

**关键实现细节**：
- `tool_calls[].function.arguments` 在 OpenAI 格式中必须是 JSON **字符串**（如 `"{\"expr\":\"1+1\"}"`），而非 JSON 对象。代码通过 `tool_input.dump()` 将参数对象序列化为字符串
- tool 消息的 `content` 在 OpenAI 格式中为纯字符串，`tool_call_id` 与 `role` 平级

### 4. `src/core/agent_loop.cpp` — 响应解析格式迁移

| 函数 | 改动 |
|------|------|
| `has_tool_use()` | 优先检查 OpenAI 格式 `message.tool_calls` 数组，兼容 Anthropic content 块格式 |
| `extract_tool_use()` | 从 `choices[0].message.tool_calls[0]` 提取工具调用，转换 `function.name` → `name`，解析 `function.arguments` JSON 字符串 → `input` 对象 |
| `extract_text()` | 优先读取 `choices[0].message.content` 字符串，兼容 Anthropic content 数组格式 |
| `should_stop()` | 优先检查 `choices[0].finish_reason`（`"stop"` / `"tool_calls"` / `"length"`），兼容 Anthropic `stop_reason` 格式 |

**新增辅助函数**：
- `get_message_from_response()` — 从 OpenAI 响应中提取 `choices[0].message`，如不存在则回退到根对象（Anthropic 兼容）。4 个解析函数均通过此辅助函数统一入口

## 验证结果

### 单元测试
```
100% tests passed, 0 tests failed out of 6
  test_logger          Passed
  test_config          Passed
  test_conversation    Passed
  test_tool_registry   Passed
  test_agent_loop      Passed
  test_luau_engine     Passed
```

### 端到端测试
```
$ ANTHROPIC_AUTH_TOKEN=sk-test123 \
  ANTHROPIC_BASE_URL=https://api.deepseek.com \
  ./ai_agent.exe -n "hello"

→ 返回 401 认证错误（使用假密钥，预期结果）
→ 错误消息为 "Authentication Fails"，非 400 格式错误
```

修复前返回 `400 - missing field 'type'`，修复后不再出现此错误，请求体格式已被 DeepSeek API 正确接受。

### 编译
- 零错误、零警告

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `src/common/constants.hpp` | 修改 — 新增 OpenAI 格式常量声明 |
| `src/common/constants.cpp` | 修改 — 新增 OpenAI 格式常量定义 |
| `src/core/api/anthropic_client.cpp` | 修改 — 请求体从 Anthropic 格式迁移为 OpenAI 格式 |
| `src/core/agent_loop.cpp` | 修改 — 响应解析从 Anthropic 格式迁移为 OpenAI 格式 |
| `changelog/FIXES_4_CHANGELOG.md` | 新建 |
