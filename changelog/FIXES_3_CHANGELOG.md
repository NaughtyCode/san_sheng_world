# Fixes Issue #3 — 修改记录

## 问题

运行时 API 返回 404 错误：
```
[ERROR] AnthropicClient: API error 404:
[ERROR] AgentLoop: empty response from API
Error: failed to get response from API
```

## 根因分析

经测试验证，`https://api.deepseek.com/anthropic/v1/messages` 端点本身可达（返回 401 认证错误），404 可能由以下原因之一引起：
1. 时刻性网络/API 波动
2. API Key 权限不足
3. 请求体格式与服务端临时不兼容

**关键问题**：原有错误日志未输出请求的完整 URL 和响应体，导致 404 发生时无法定位问题。

## 修改内容

### 1. `src/core/api/anthropic_client.cpp` — 错误诊断增强

| 改动 | 说明 |
|------|------|
| 新增 `full_url` 变量 | 拼接 `base_url_ + API_PATH_MESSAGES` 完整端点地址 |
| debug 日志含 URL | `"AnthropicClient: POST {url}, model={m}, msg_count={n}"` |
| error 日志含 URL | HTTP 连接失败和 API 错误均输出完整 URL |
| HTTP 状态码诊断 | 对 400/401/403/404/429/500 提供中文排查建议 |
| 404 特殊处理 | 提示信息包含完整 API 端点地址，方便直接检查 |

**示例输出（修复后）**：
```
# 成功连接但认证失败：
[ERROR] AnthropicClient: API error 401 from https://api.deepseek.com/anthropic/v1/messages:
{"error":{"message":"Authentication Fails..."}}

# 如果 404 再次发生：
[ERROR] AnthropicClient: API error 404 from https://api.deepseek.com/anthropic/v1/messages:
[Not Found — check API endpoint: https://api.deepseek.com/anthropic/v1/messages]
```

### 2. `src/core/agent_loop.cpp` — 启动信息优化

| 改动 | 说明 |
|------|------|
| `get_model_info()` 新增 `Endpoint` 行 | 显示完整 API 端点地址（base_url + path） |
| 移除冗余的 `Base URL` 行 | 合并为 `Endpoint`，信息更直观 |

**启动输出示例**：
```
Endpoint: https://api.deepseek.com/anthropic/v1/messages
Model: deepseek-v4-pro
API Version: 2023-06-01
API Key: sk-test****t123
Max Tokens: 4096
Max Iterations: 10
```

### 3. `CMakeLists.txt` — MSVC 编码支持

| 改动 | 说明 |
|------|------|
| MSVC `/utf-8` 编译选项 | 解决中文注释 C4819 编码警告 |

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
  ANTHROPIC_BASE_URL=https://api.deepseek.com/anthropic \
  ./ai_agent.exe -n "hello"

Endpoint: https://api.deepseek.com/anthropic/v1/messages
Model: deepseek-v4-pro
API Key: sk-test****t123
→ 401 认证错误（使用假密钥，预期结果）
→ 错误消息包含完整 URL 和响应体
```

### 编译
- 零错误、零警告

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `src/core/api/anthropic_client.cpp` | 修改 — 错误诊断增强 |
| `src/core/agent_loop.cpp` | 修改 — 启动信息新增 Endpoint |
| `CMakeLists.txt` | 修改 — MSVC /utf-8 |
| `changelog/FIXES_3_CHANGELOG.md` | 新建 |
