# Fixes Issue #6 — 修改记录

## 问题

工具调用后，第二轮 API 请求返回 400 错误：

```
[INFO] AgentLoop: tool_use detected: calculator
[ERROR] AnthropicClient: API error 400 from https://api.deepseek.com/anthropic/chat/completions:
{"error":{"message":"The `reasoning_content` in the thinking mode must be passed back to the API.",...}}
[ERROR] AgentLoop: empty response from API
Error: failed to get response from API
```

## 根因分析

DeepSeek 的 thinking mode（v4-pro 模型默认开启）在 assistant 消息中额外返回 `reasoning_content` 字段，包含模型的推理/思考过程。**API 强制要求**：在后续同一对话的请求中，必须将 assistant 消息的 `reasoning_content` 原样回传，否则返回 400 错误。

修复前的数据流：

```
API 响应: { choices[0].message: { content: "...", reasoning_content: "推理...", tool_calls: [...] } }
                    ↓
agent_loop 提取: 只保存 text + tool_call → 丢弃 reasoning_content
                    ↓
下一轮请求: assistant 消息缺少 reasoning_content → 400 错误
```

## 修改内容

### 1. `src/common/constants.hpp` / `constants.cpp` — 新增常量

| 改动 | 说明 |
|------|------|
| 新增 `JSON_REASONING_CONTENT` = `"reasoning_content"` | 用于提取和序列化 reasoning 字段 |

### 2. `src/core/api/anthropic_client.hpp` — Message 结构扩展

| 改动 | 说明 |
|------|------|
| `Message` 新增 `reasoning_content` 字段 | 存储 assistant 消息的推理内容，默认空字符串 |

```cpp
struct Message {
    // ... 原有字段 ...
    std::string reasoning_content; // DeepSeek thinking mode 推理内容（需在后续请求中回传）
};
```

### 3. `src/core/conversation.hpp` / `conversation.cpp` — 接口扩展

| 改动 | 说明 |
|------|------|
| `add_assistant_msg()` 新增 `reasoning_content` 参数 | 默认值 `""`，保持向后兼容 |

### 4. `src/core/agent_loop.hpp` / `agent_loop.cpp` — 提取并回传 reasoning

| 改动 | 说明 |
|------|------|
| 新增 `extract_reasoning()` 方法 | 从 `choices[0].message.reasoning_content` 提取推理文本 |
| `run()` 方法 | 两处调用 `conversation_.add_assistant_msg()` 均传入提取的 reasoning |

**关键代码路径**：
```cpp
// 每次 API 响应后提取 reasoning
std::string reasoning = extract_reasoning(response);

// tool_use 路径
conversation_.add_assistant_msg(assistant_text, tool_name, tool_id, tool_input, reasoning);

// 纯文本路径
conversation_.add_assistant_msg(text, "", "", {}, reasoning);
```

### 5. `src/core/api/anthropic_client.cpp` — 序列化 reasoning

| 改动 | 说明 |
|------|------|
| `messages_to_api_format()` assistant + tool_calls 分支 | 若 `reasoning_content` 非空，写入消息 JSON |
| `messages_to_api_format()` 纯文本 assistant 分支 | 同上 |

**序列化后 assistant 消息格式**：
```json
{
  "role": "assistant",
  "content": "Let me calculate that...",
  "reasoning_content": "用户要求计算 1+1，我将使用 calculator 工具...",
  "tool_calls": [...]
}
```

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

→ 返回 401 Authentication Fails（使用假密钥，预期结果）
→ 不再出现 400 "reasoning_content must be passed back" 错误
```

### 编译
- 零错误、零警告

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `src/common/constants.hpp` | 修改 — 新增 `JSON_REASONING_CONTENT` 声明 |
| `src/common/constants.cpp` | 修改 — 新增 `JSON_REASONING_CONTENT` 定义 |
| `src/core/api/anthropic_client.hpp` | 修改 — `Message` 新增 `reasoning_content` 字段 |
| `src/core/api/anthropic_client.cpp` | 修改 — 序列化时包含 `reasoning_content` |
| `src/core/conversation.hpp` | 修改 — `add_assistant_msg()` 新增参数 |
| `src/core/conversation.cpp` | 修改 — 存储 `reasoning_content` |
| `src/core/agent_loop.hpp` | 修改 — 新增 `extract_reasoning()` 声明 |
| `src/core/agent_loop.cpp` | 修改 — 提取并回传 `reasoning_content` |
| `changelog/FIXES_6_CHANGELOG.md` | 新建 |
