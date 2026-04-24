# Fixes Issue #2 — 修改记录

## 概述

本次变更实现两大目标：
1. **支持新的环境变量**，扩展工程配置能力
2. **常量集中管理**，消除分散在代码各处的魔法数字和硬编码字符串

---

## 新增文件

### `src/common/constants.hpp` — 常量声明头文件

集中声明工程中的所有常量（约 60 个），按类别分组：

| 分组 | 数量 | 说明 |
|------|------|------|
| 环境变量名称 | 9 | `ENV_ANTHROPIC_API_KEY`, `ENV_ANTHROPIC_AUTH_TOKEN`, `ENV_ANTHROPIC_MODEL` 等 |
| 配置键 | 9 | `CONFIG_API_KEY`, `CONFIG_MODEL`, `CONFIG_TIMEOUT_MS` 等 |
| 默认值 | 10 | `DEFAULT_MODEL`, `DEFAULT_TIMEOUT_MS` 等 |
| API 通用字符串 | 5 | `API_HEADER_CONTENT_TYPE`, `API_PATH_MESSAGES` 等 |
| JSON 键 | 19 | `JSON_MODEL`, `JSON_CONTENT`, `JSON_TYPE` 等 |
| JSON 值常量 | 8 | `VALUE_TOOL_USE`, `VALUE_END_TURN` 等 |

所有常量位于 `namespace agent::constants`，使用 `extern const` 声明。

### `src/common/constants.cpp` — 常量定义文件

唯一定义所有常量的翻译单元。任何常量修改只需在此文件中操作。

---

## 修改的文件

### 1. `CMakeLists.txt`（根）

- **新增**: 为 MSVC 编译器添加 `/utf-8` 选项，解决中文注释的 C4819 编码警告

### 2. `src/common/CMakeLists.txt`

- **新增**: `constants.cpp` 加入 `agent_common` 静态库编译列表

### 3. `src/common/config.cpp` — 全面重写

**新增环境变量支持**:

| 环境变量 | 配置键 | 说明 |
|----------|--------|------|
| `ANTHROPIC_SMALL_FAST_MODEL` | `small_fast_model` | 小型快速模型名称 |
| `ANTHROPIC_CUSTOM_MODEL_OPTION` | `custom_model_option` | 自定义模型选项 |
| `ANTHROPIC_AUTH_TOKEN` | `auth_token` | 认证令牌（备用凭证） |
| `API_TIMEOUT_MS` | `timeout_ms` | API 超时时间（毫秒） |

- **改用常量**: 所有字符串字面量替换为 `constants::XXX` 引用
- **添加注释**: 每个环境变量的用途说明

### 4. `src/core/api/http_client.hpp`

- **改用常量**: `#include "constants.hpp"`，默认超时使用 `constants::DEFAULT_TIMEOUT_MS / 1000`

### 5. `src/core/api/http_client.cpp`

- **改用常量**: `"application/json"` → `constants::API_MIME_JSON`

### 6. `src/core/api/anthropic_client.hpp`

- **改用常量**: 所有默认值使用 `constants::DEFAULT_MODEL`, `constants::DEFAULT_BASE_URL`, `constants::DEFAULT_API_VERSION` 等
- **新增**: `set_timeout(int seconds)` 方法，用于设置 HTTP 超时
- **新增**: Doxygen 风格注释

### 7. `src/core/api/anthropic_client.cpp`

- **改用常量**: 所有 JSON 键（`model`, `max_tokens`, `messages`, `tools`, `role`, `content`, `type`, `text`, `name`, `description`, `input_schema`, `input`, `id`, `tool_use_id`, `stop_reason` 等）替换为 `constants::JSON_XXX`
- **改用常量**: 所有固定值（`tool_use`, `tool_result`, `user`, `assistant`, `tool` 等）替换为 `constants::VALUE_XXX`
- **改用常量**: HTTP 头名称和路径使用 `constants::API_XXX`

### 8. `src/core/agent_loop.hpp`

- **改用常量**: 默认值 `max_tokens_`, `max_iterations_` 使用 `constants::DEFAULT_MAX_TOKENS`, `constants::DEFAULT_MAX_ITERATIONS`
- **新增**: `set_base_url()`, `set_timeout()` 方法
- **新增**: Doxygen 注释

### 9. `src/core/agent_loop.cpp`

- **改用常量**: `load_builtin_tools()`, `run()`, `has_tool_use()`, `extract_tool_use()`, `extract_text()`, `should_stop()` 中所有 JSON 键值替换为常量引用
- **添加注释**: `using namespace constants;` 局部引入

### 10. `src/main/main.cpp` — 全面重写

- **认证策略变更**:
  ```
  优先读取 ANTHROPIC_API_KEY → 若空则读取 ANTHROPIC_AUTH_TOKEN → 仍为空则报错退出
  ```
  错误信息同时提示两个环境变量名称，方便用户排查

- **新增配置读取**:
  - `ANTHROPIC_BASE_URL` → 设置 `agent.set_base_url()`
  - `API_TIMEOUT_MS` → 转换为秒后调用 `agent.set_timeout()`
  - `ANTHROPIC_SMALL_FAST_MODEL` → 读入 config 备用
  - `ANTHROPIC_CUSTOM_MODEL_OPTION` → 读入 config 备用

- **改用常量**: 所有默认值、配置键使用 `constants::XXX`

---

## 功能变更总结

### 支持的环境变量（完整列表）

| 环境变量 | 默认值 | 用途 |
|----------|--------|------|
| `ANTHROPIC_API_KEY` | — | 主要 API 认证密钥（优先） |
| `ANTHROPIC_AUTH_TOKEN` | — | 备用认证令牌 |
| `ANTHROPIC_MODEL` | `deepseek-v4-pro` | 主模型名称 |
| `ANTHROPIC_SMALL_FAST_MODEL` | `deepseek-v4-pro` | 小型快速模型（预留） |
| `ANTHROPIC_CUSTOM_MODEL_OPTION` | `deepseek-v4-pro` | 自定义模型选项（预留） |
| `ANTHROPIC_BASE_URL` | `https://api.deepseek.com/anthropic` | API 端点 URL |
| `API_TIMEOUT_MS` | `600000` | 请求超时（毫秒） |
| `LOG_LEVEL` | `info` | 日志级别 |
| `LOG_FILE` | — | 日志文件路径 |

### 默认值变更

| 参数 | 旧值 | 新值 |
|------|------|------|
| 默认模型 | `claude-sonnet-4-6` | `deepseek-v4-pro` |
| 默认 Base URL | `https://api.anthropic.com` | `https://api.deepseek.com/anthropic` |
| 默认超时 | 30 秒 | 600 秒 (600000ms) |

---

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
# 场景 1：使用 ANTHROPIC_AUTH_TOKEN 认证
$ ANTHROPIC_AUTH_TOKEN=sk-ant-test-key ANTHROPIC_BASE_URL=https://api.deepseek.com/anthropic ./ai_agent.exe -n "hello"
Model: deepseek-v4-pro
Base URL: https://api.deepseek.com/anthropic
API Version: 2023-06-01
API Key: sk-ant-****-key
Max Tokens: 4096
Max Iterations: 10
→ 连接成功，返回 401 认证错误（预期，使用假密钥）

# 场景 2：ANTHROPIC_API_KEY 优先于 ANTHROPIC_AUTH_TOKEN
$ ANTHROPIC_API_KEY=sk-ant-primary-key ANTHROPIC_AUTH_TOKEN=sk-ant-fallback ./ai_agent.exe -n "test"
API Key: sk-ant-****-key
→ 使用 API_KEY 而非 AUTH_TOKEN ✓
```

### 编译

- 零错误、零警告
- MSVC `/utf-8` 选项解决中文注释编码问题
