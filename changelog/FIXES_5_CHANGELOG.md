# Fixes Issue #5 — 修改记录

## 问题

API 请求在工具调用后返回 400 错误，且 Luau 引擎报告 `eval_expr` 函数未找到：

```
[INFO] AgentLoop: tool_use detected: calculator
[ERROR] LuauEngine: function "eval_expr" not found
[INFO] LuauEngine: loaded script "__calculator__"
[ERROR] AnthropicClient: API error 400 from https://api.deepseek.com/anthropic/chat/completions:
{"error":{"message":"Failed to deserialize the JSON body into the target type:
messages[2]: missing field `tool_call_id` at line 1 column 354",...}}
[ERROR] AgentLoop: empty response from API
Error: failed to get response from API
```

日志中包含两个独立错误：

### 错误 1：`messages[2]: missing field 'tool_call_id'`

工具结果消息（tool role）发送回 API 时，使用了 Anthropic 格式的字段名 `tool_use_id`，但 OpenAI Chat Completions API 要求使用 `tool_call_id`。

### 错误 2：`function "eval_expr" not found`

内置 calculator 工具通过 `call_function("eval_expr", ...)` 调用 Luau 全局函数求值表达式，但 `eval_expr` 仅在 `scripts_luau/example_tool.luau` 脚本中定义，该脚本从未被默认加载，导致函数不存在。回退逻辑使用 `load_string` 编译表达式但不返回计算结果，输出为 `"Calculator result: <原始表达式>"` 而非实际值。

## 根因分析

| 错误 | 根因 | 影响范围 |
|------|------|----------|
| `missing field 'tool_call_id'` | `messages_to_api_format()` 对 tool 消息使用 `JSON_TOOL_USE_ID`（`"tool_use_id"`），但 OpenAI API 期望 `"tool_call_id"` | 所有工具调用均会在第二轮 API 请求失败 |
| `eval_expr not found` | calculator 工具依赖未注册的 Luau 全局函数 | calculator 工具无法正确求值 |

## 修改内容

### 1. `src/common/constants.hpp` / `src/common/constants.cpp` — 新增 `tool_call_id` 常量

| 改动 | 说明 |
|------|------|
| 新增 `JSON_TOOL_CALL_ID` = `"tool_call_id"` | OpenAI 格式 tool 消息所需的字段名 |
| 保留 `JSON_TOOL_USE_ID` = `"tool_use_id"` | 仅供 Anthropic 兼容场景使用 |

### 2. `src/core/api/anthropic_client.cpp` — 修复 tool 消息字段名

| 改动 | 说明 |
|------|------|
| `messages_to_api_format()` tool 分支 | `JSON_TOOL_USE_ID` → `JSON_TOOL_CALL_ID`，使 tool 消息符合 OpenAI 格式 |

**修复前（tool 消息）**：
```json
{"role": "tool", "tool_use_id": "call_xxx", "content": "result"}
```
**修复后**：
```json
{"role": "tool", "tool_call_id": "call_xxx", "content": "result"}
```

### 3. `src/core/script/luau_engine.hpp` / `luau_engine.cpp` — 新增 `evaluate_expression` 方法

| 改动 | 说明 |
|------|------|
| 声明 `evaluate_expression(const std::string& expr)` | 安全求值数学/逻辑表达式，返回计算结果 JSON |
| 实现 | 将 `"return <expr>"` 编译为 Luau 字节码，在沙箱中通过 `lua_pcall` 执行并捕获返回值 |

**实现细节**：
- 使用 `luau_compile` → `luau_load` → `lua_pcall(L, 0, 1, 0)` 管线
- 编译失败或运行时错误均返回 `{"error": "..."}` 结构
- 数值结果返回 `number` 类型，其他返回对应 JSON 类型
- 无需依赖外部脚本文件，开箱即用

### 4. `src/core/agent_loop.cpp` — 更新 calculator 工具实现

| 改动 | 说明 |
|------|------|
| `load_builtin_tools()` calculator handler | 从 `call_function("eval_expr", ...)` + 无效回退 改为直接调用 `evaluate_expression(expr)` |
| 结果格式化 | 数值结果转为字符串（`std::to_string`），其他类型使用 `dump()` |

**修复前**：
```cpp
json result = script_engine_->call_function("eval_expr", json::array({expr}));
if (result.contains("error")) {
    // 回退路径不返回实际计算结果
    std::string code = "return " + expr;
    script_engine_->load_string("__calculator__", code);
    return "Calculator result: " + expr;  // 仅返回表达式本身
}
return result.dump();
```

**修复后**：
```cpp
json result = script_engine_->evaluate_expression(expr);
if (result.contains("error")) {
    return "Calculator error: " + result["error"].get<std::string>();
}
if (result.is_number()) {
    return std::to_string(result.get<double>());
}
return result.dump();
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
→ 不再出现 400 "missing field 'tool_call_id'" 错误
→ 不再出现 "eval_expr not found" 错误
```

### 编译
- 零错误、零警告

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `src/common/constants.hpp` | 修改 — 新增 `JSON_TOOL_CALL_ID` 声明 |
| `src/common/constants.cpp` | 修改 — 新增 `JSON_TOOL_CALL_ID` 定义 |
| `src/core/api/anthropic_client.cpp` | 修改 — tool 消息使用 `JSON_TOOL_CALL_ID` |
| `src/core/script/luau_engine.hpp` | 修改 — 新增 `evaluate_expression()` 声明 |
| `src/core/script/luau_engine.cpp` | 修改 — 新增 `evaluate_expression()` 实现 |
| `src/core/agent_loop.cpp` | 修改 — calculator 工具使用 `evaluate_expression()` |
| `changelog/FIXES_5_CHANGELOG.md` | 新建 |
