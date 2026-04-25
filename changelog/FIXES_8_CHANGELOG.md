# Fixes Issue #8 — 重构大模型返回信息处理逻辑

## 概述

将大模型 API 返回信息的处理逻辑从 `agent_loop` 和 `anthropic_client` 中解耦，抽象为独立的 `process_engine` 模块。模块内部按职责分为两类独立组件，各自使用专属日志标签便于追踪。

## 旧架构问题

- 响应解析逻辑（`get_message_from_response`、`has_tool_use`、`extract_tool_use`、`extract_text`、`extract_reasoning`、`should_stop`）全部内嵌在 `AgentLoop` 的私有方法中（`agent_loop.cpp` 第 174-317 行）
- 请求格式化逻辑（`build_request_body`、`messages_to_api_format`）内嵌在 `AnthropicClient` 的私有方法中（`anthropic_client.cpp` 第 68-148 行）
- 两类逻辑职责不清，修改一处可能影响另一处
- 无独立的日志标签，难以追踪处理链路
- 单元测试困难（需实例化整个 AgentLoop 或 AnthropicClient）

## 新架构

```
src/core/process_engine/
├── response_parser.hpp/cpp    — 响应解析：提取 text / tool_use / reasoning / stop 判断
└── message_formatter.hpp/cpp  — 请求格式化：Message → API JSON，ToolDefinition → API JSON
```

### 职责分离

| 组件 | 职责 | 日志标签 |
|------|------|---------|
| `ResponseParser` | 从 API 响应 JSON 中解析结构化信息 | `[ProcessEngine::ResponseParser]` |
| `MessageFormatter` | 将内部数据结构转换为 API 兼容的请求体 | `[ProcessEngine::MessageFormatter]` |

## 修改内容

### 1. 新增 `src/core/process_engine/response_parser.hpp`（79 行）

`ResponseParser` 类定义，所有方法均为 `static`（无状态、纯函数）：

| 方法 | 说明 |
|------|------|
| `get_message(response)` | 从 API 响应中提取消息对象（兼容 OpenAI / Anthropic 格式） |
| `has_tool_use(response)` | 检查响应是否包含工具调用 |
| `extract_tool_use(response)` | 提取工具调用信息为统一格式 `{id, name, input}` |
| `extract_text(response)` | 提取纯文本内容 |
| `extract_reasoning(response)` | 提取 DeepSeek thinking mode 推理内容 |
| `should_stop(response)` | 判断对话是否应结束 |

### 2. 新增 `src/core/process_engine/response_parser.cpp`（205 行）

完整实现所有响应解析方法，每个方法包含：
- OpenAI Chat Completions 格式解析分支
- Anthropic Messages 原始格式兼容分支
- `[ProcessEngine::ResponseParser]` 标签的调试日志

### 3. 新增 `src/core/process_engine/message_formatter.hpp`（65 行）

`MessageFormatter` 类定义：

| 方法 | 说明 |
|------|------|
| `build_request_body(model, msgs, tools, max_tokens)` | 构造完整 API 请求体 |
| `to_api_format(msgs)` | 将 `vector<Message>` 转换为 API JSON 数组 |
| `format_tools(tools)` | 将 `vector<ToolDefinition>` 转换为 API JSON 数组 |

### 4. 新增 `src/core/process_engine/message_formatter.cpp`（130 行）

完整实现请求格式化，处理三种消息类型的转换：
- **tool 消息**：含 `tool_call_id` + `content`（纯字符串）
- **assistant（含 tool_use）消息**：含 `tool_calls` 数组，`arguments` 为 JSON 字符串
- **纯文本消息**：`content` 为纯字符串

DeepSeek thinking mode 的 `reasoning_content` 在所有 assistant 消息中正确回传。

### 5. `src/core/agent_loop.hpp` — 移除私有解析方法

| 改动 | 说明 |
|------|------|
| 移除 `has_tool_use()` 声明 | 迁移至 `ResponseParser::has_tool_use()` |
| 移除 `extract_tool_use()` 声明 | 迁移至 `ResponseParser::extract_tool_use()` |
| 移除 `extract_text()` 声明 | 迁移至 `ResponseParser::extract_text()` |
| 移除 `extract_reasoning()` 声明 | 迁移至 `ResponseParser::extract_reasoning()` |
| 移除 `should_stop()` 声明 | 迁移至 `ResponseParser::should_stop()` |
| 更新 `run()` 文档注释 | 说明使用 process_engine 模块 |

### 6. `src/core/agent_loop.cpp` — 使用 ResponseParser（从 319 行缩减至 175 行）

- 新增 `#include "process_engine/response_parser.hpp"`
- 移除静态函数 `get_message_from_response`（第 175-186 行）
- 移除私有方法 `has_tool_use`（第 188-206 行）
- 移除私有方法 `extract_tool_use`（第 209-249 行）
- 移除私有方法 `extract_text`（第 252-275 行）
- 移除私有方法 `extract_reasoning`（第 277-286 行）
- 移除私有方法 `should_stop`（第 288-317 行）
- `run()` 中全部替换为 `ResponseParser::xxx()` 调用

**典型变更**：
```cpp
// 旧代码
std::string reasoning = extract_reasoning(response);
if (should_stop(response)) { ... }
if (has_tool_use(response)) { ... }

// 新代码
std::string reasoning = ResponseParser::extract_reasoning(response);
if (ResponseParser::should_stop(response)) { ... }
if (ResponseParser::has_tool_use(response)) { ... }
```

### 7. `src/core/api/anthropic_client.hpp` — 移除私有格式化方法

| 改动 | 说明 |
|------|------|
| 移除 `build_request_body()` 声明 | 迁移至 `MessageFormatter::build_request_body()` |
| 移除 `messages_to_api_format()` 声明 | 迁移至 `MessageFormatter::to_api_format()` |
| 更新 `messages_create()` 文档注释 | 说明使用 MessageFormatter |
| 补充 `Message` / `ToolDefinition` 结构体注释 | 增加中文说明 |

### 8. `src/core/api/anthropic_client.cpp` — 使用 MessageFormatter（从 150 行缩减至 80 行）

- 新增 `#include "process_engine/message_formatter.hpp"`
- 移除 `build_request_body()` 方法（第 68-99 行）
- 移除 `messages_to_api_format()` 方法（第 102-148 行）
- `messages_create()` 中替换为 `MessageFormatter::build_request_body()`

### 9. `src/core/CMakeLists.txt` — 新增编译目标

```cmake
# 新增两行
process_engine/response_parser.cpp
process_engine/message_formatter.cpp
```

## 设计原则

### 无状态静态方法
`ResponseParser` 和 `MessageFormatter` 的所有方法均为 `static`，不持有任何成员变量。这带来以下优势：
- **线程安全**：多线程同时调用无需同步
- **零开销**：无需实例化，直接类名调用
- **可测试性**：输入→输出纯函数，不依赖外部状态
- **组合性**：可在任何上下文中独立使用

### 模块专属日志标签
每个组件的日志输出使用专属标签，便于过滤和追踪：
```
[ProcessEngine::ResponseParser] detected tool_use via OpenAI tool_calls
[ProcessEngine::ResponseParser] finish_reason=stop → should stop
[ProcessEngine::MessageFormatter] built request: model=deepseek-v4-pro, msg_count=3
[ProcessEngine::MessageFormatter] formatted 1 tool definitions
```

### 双格式兼容
所有解析方法同时支持 OpenAI Chat Completions 和 Anthropic Messages 两种 API 格式，通过 `get_message()` 统一入口后分派处理。

## 验证结果

### 编译

```
零错误、零警告
3 个新文件编译成功
process_engine → agent_core.lib 链接成功
```

### 单元测试

```
100% tests passed, 0 tests failed out of 6
  test_logger          Passed
  test_config          Passed
  test_conversation    Passed
  test_tool_registry   Passed
  test_agent_loop      Passed (使用 ResponseParser 的完整循环)
  test_luau_engine     Passed
```

### 端到端测试

```
$ ./build/bin/Release/ai_agent.exe -n "test"

[2026-04-25 09:19:24.565] [info] [20948] [:] AI Agent starting
[2026-04-25 09:19:24.565] [info] [20948] [:] LuauEngine: initialized with sandbox and log API
[2026-04-25 09:19:24.565] [info] [20948] [:] ToolRegistry: registered tool "calculator"
[2026-04-25 09:19:24.565] [info] [20948] [:] Agent initialized with model: deepseek-v4-pro
...
→ 正常启动，日志输出包含 process_engine 模块标签
```

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `src/core/process_engine/response_parser.hpp` | 新增 — 响应解析器头文件 |
| `src/core/process_engine/response_parser.cpp` | 新增 — 响应解析器实现（205 行） |
| `src/core/process_engine/message_formatter.hpp` | 新增 — 消息格式化器头文件 |
| `src/core/process_engine/message_formatter.cpp` | 新增 — 消息格式化器实现（130 行） |
| `src/core/agent_loop.hpp` | 修改 — 移除私有解析方法声明，更新文档 |
| `src/core/agent_loop.cpp` | 修改 — 移除解析实现，改用 ResponseParser（-144 行） |
| `src/core/api/anthropic_client.hpp` | 修改 — 移除私有格式化方法，补充注释 |
| `src/core/api/anthropic_client.cpp` | 修改 — 移除格式化实现，改用 MessageFormatter（-70 行） |
| `src/core/CMakeLists.txt` | 修改 — 新增 2 个 process_engine 源文件 |
| `changelog/FIXES_8_CHANGELOG.md` | 新增 |
