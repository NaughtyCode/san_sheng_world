# Fixes Issue #7 — 替换工程日志系统

## 概述

将项目旧的 `agent::Logger` 日志系统替换为基于 [spdlog](https://github.com/gabime/spdlog) 的全新异步日志系统 `LogManager`，支持彩色控制台输出、文件轮转、按模块分离日志、JSON 配置，并将新日志 API 注册到 LuauEngine 虚拟机中。

## 旧日志系统问题

- 基于 `printf` 风格格式化，无结构化输出能力
- 仅支持控制台或文件二选一输出
- 无日志级别彩色显示
- 无日志文件轮转功能（长时间运行会占满磁盘）
- 同步写入，高负载时阻塞主线程
- Luau 脚本无法输出日志

## 新日志系统特性

| 特性 | 说明 |
|------|------|
| **异步日志** | 后台线程写入，队列长度 8192，不阻塞主线程 |
| **彩色控制台** | ERROR=红色, WARN=黄色, INFO=绿色, DEBUG=青色, TRACE=灰色 |
| **文件轮转** | 日志文件按大小自动轮转，保留历史文件 |
| **多模块隔离** | 支持按模块创建独立 logger（如 database 模块） |
| **JSON 配置** | 通过 `log_config.json` 配置日志路径、大小、级别 |
| **运行时动态调整** | 无需重启即可修改日志级别 |
| **Luau 脚本集成** | Luau 脚本可调用 `log_info()` 等函数输出日志 |
| **fmt 格式化** | 使用 spdlog 内置的 fmt 库，性能远超 printf |

## 修改内容

### 1. 新增 spdlog 依赖（git submodule）

```bash
git submodule add https://github.com/gabime/spdlog.git external/spdlog
```

**`.gitmodules`** — 自动更新，添加 spdlog 子模块条目。

### 2. `external/CMakeLists.txt` — 添加 spdlog 构建

| 改动 | 说明 |
|------|------|
| 新增 `add_subdirectory(external/spdlog EXCLUDE_FROM_ALL)` | 将 spdlog 加入构建系统 |

### 3. `src/common/logger.hpp` — 完全重写

**旧文件**（39 行）：定义 `agent::Logger` 类，包含 `debug/info/warning/error` 方法，使用 `printf` 风格格式化。

**新文件**（42 行）：提供便捷的 `LOG_*` 宏，直接使用 `LogManager` + spdlog：

```cpp
#define LOG_TRACE(...)    ::LogManager::instance().get_logger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::LogManager::instance().get_logger()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::LogManager::instance().get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::LogManager::instance().get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::LogManager::instance().get_logger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::LogManager::instance().get_logger()->critical(__VA_ARGS__)
```

### 4. `src/common/logger.cpp` — 简化为占位文件

旧的 99 行实现（`Logger::instance()`, `set_level()`, `set_file()`, `log()`, `debug()`, `info()`, `warning()`, `error()`, `write()`）全部移除。新文件仅保留头文件包含和注释。

### 5. `src/log/log_manager.h` — 新增详细注释

全面补充中文注释，包含：
- 类级别功能说明
- 每个公开方法的 `@brief`/`@param`/`@return` 文档
- 每个私有方法和成员变量的用途说明
- 配置结构体字段含义注释

### 6. `src/log/log_manager.cpp` — 新增详细注释

全面补充中文注释，包含：
- 模块级功能概述
- 每个函数的算法说明
- 关键设计决策的 WHY 注释（如队列长度 8192、block 策略、spdlog 析构顺序）

### 7. `src/common/CMakeLists.txt` — 更新编译配置

| 改动 | 说明 |
|------|------|
| 新增 `../log/log_manager.cpp` 源文件 | 将 LogManager 编译进 agent_common |
| 新增 `${CMAKE_SOURCE_DIR}/src/log` 包含路径 | 使 log_manager.h 可被引用 |
| 新增 `spdlog::spdlog` 链接依赖 | 链接 spdlog 库 |

### 8. 所有源文件 — 替换日志调用

所有使用旧 `Logger::instance().xxx()` 的文件均已更新为新的 `LOG_*()` 宏，格式化字符串从 `printf` 风格改为 `fmt` 风格。

| 文件 | 改动数量 | 典型变更 |
|------|---------|---------|
| `src/main/main.cpp` | 6 处 | `Logger::instance().info("msg %s", x)` → `LOG_INFO("msg {}", x)` |
| `src/core/script/luau_engine.cpp` | 18 处 | 全部 Logger 调用替换为 LOG_* 宏 |
| `src/core/agent_loop.cpp` | 5 处 | 同上 |
| `src/core/tool_registry.cpp` | 4 处 | 同上 |
| `src/core/api/anthropic_client.cpp` | 4 处 | 同上 |
| `src/core/api/http_client.cpp` | 3 处 | 同上 |

**格式化风格变更示例**：

```cpp
// 旧风格 (printf)
Logger::instance().info("Agent initialized with model: %s", model.c_str());
Logger::instance().debug("Iteration %d/%d", iter + 1, max_iterations_);

// 新风格 (fmt)
LOG_INFO("Agent initialized with model: {}", model);
LOG_DEBUG("Iteration {}/{}", iter + 1, max_iterations_);
```

### 9. `src/main/main.cpp` — 初始化流程更新

| 改动 | 说明 |
|------|------|
| 新增 `LogManager::instance().init("log_config.json", "APP_LOG_FILE")` | 启动时初始化日志系统 |
| 替换 `std::cerr` → `LOG_CRITICAL()` | API 密钥缺失时使用日志系统输出 |
| 新增 `LogManager::instance().shutdown()` | 退出前优雅关闭 |
| 新增日志配置与动态级别设置 | 从环境变量读取日志级别后动态调整 |

### 10. `src/core/script/luau_engine.hpp` / `.cpp` — 注册日志 API

**新增方法**：`LuauEngine::register_log_api()`

在 Luau 虚拟机中注册 4 个全局日志函数：

| Luau 函数 | 对应 C++ 宏 | 日志级别 |
|-----------|------------|---------|
| `log_debug(msg)` | `LOG_DEBUG("[Luau] {}", msg)` | DEBUG |
| `log_info(msg)` | `LOG_INFO("[Luau] {}", msg)` | INFO |
| `log_warn(msg)` | `LOG_WARN("[Luau] {}", msg)` | WARN |
| `log_error(msg)` | `LOG_ERROR("[Luau] {}", msg)` | ERROR |

所有 Luau 日志消息自动添加 `[Luau]` 前缀以区分来源。

```cpp
// 注册示例
lua_pushcfunction(L_, [](lua_State* L) -> int {
    const char* msg = luaL_checkstring(L, 1);
    LOG_INFO("[Luau] {}", msg);
    return 0;
}, "log_info");
lua_setglobal(L_, "log_info");
```

Luau 脚本使用示例：
```lua
log_info("Script started")
log_debug("Processing item: " .. item_name)
log_warn("Memory usage is high")
log_error("Failed to connect to server")
```

### 11. `tests/test_common/test_logger.cpp` — 重写测试

旧测试针对 `agent::Logger` 类，已全部重写为针对新 `LogManager` 的测试：

| 测试用例 | 测试内容 |
|---------|---------|
| `Singleton` | LogManager 单例唯一性 |
| `InitWithDefaults` | 默认配置初始化 |
| `GetLogger` | logger 获取与回退机制 |
| `SetLevel` | 运行时级别修改 |
| `MacroOutputs` | 所有 LOG_* 宏不崩溃 |
| `ShutdownAndReinit` | 关闭后重新初始化 |

### 12. `log_config.json` — 新增默认配置文件

```json
{
  "global_level": "info",
  "log_file": {
    "path": "logs/app.log",
    "rotation_size_mb": 100,
    "rotation_files": 10
  }
}
```

## 删除的文件

| 文件 | 说明 |
|------|------|
| `src/common/logger.hpp`（旧内容） | 旧的 agent::Logger 类定义（已重写） |
| `src/common/logger.cpp`（旧内容） | 旧的 agent::Logger 实现（已重写） |

## 新增的文件

| 文件 | 说明 |
|------|------|
| `external/spdlog/` | spdlog git 子模块 |
| `log_config.json` | 日志系统配置文件 |

## 日志格式

新日志系统统一使用以下格式输出：

```
[2026-04-25 08:52:34.917] [info] [23224] [main.cpp:38] AI Agent starting
│                         │      │       │             └─ 日志消息
│                         │      │       └─ 源文件:行号
│                         │      └─ 线程 ID
│                         └─ 日志级别（彩色）
└─ 日期时间（精确到毫秒）
```

## 验证结果

### 编译

```
零错误、零警告
6 个目标全部构建成功
```

### 单元测试

```
100% tests passed, 0 tests failed out of 6
  test_logger          Passed (6 tests)
  test_config          Passed
  test_conversation    Passed
  test_tool_registry   Passed (4 tests)
  test_agent_loop      Passed (5 tests)
  test_luau_engine     Passed (6 tests)
```

### 端到端测试

```
$ ./build/bin/Release/ai_agent.exe -n "hello"

Endpoint: https://api.deepseek.com/anthropic/chat/completions
Model: deepseek-v4-pro
API Version: 2023-06-01
API Key: sk-0c85****ad32
Max Tokens: 4096
Max Iterations: 10
[2026-04-25 08:52:34.917] [info] [23224] [main.cpp:42] AI Agent starting
[2026-04-25 08:52:34.917] [info] [23224] [luau_engine.cpp:58] LuauEngine: initialized with sandbox and log API
[2026-04-25 08:52:34.917] [info] [23224] [tool_registry.cpp:16] ToolRegistry: registered tool "calculator"
[2026-04-25 08:52:34.917] [info] [23224] [main.cpp:89] Agent initialized with model: deepseek-v4-pro
...
```

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `.gitmodules` | 修改 — 新增 spdlog 子模块 |
| `external/spdlog/` | 新增 — spdlog 子模块 |
| `external/CMakeLists.txt` | 修改 — 添加 spdlog 子目录 |
| `src/common/CMakeLists.txt` | 修改 — 添加 log_manager.cpp 和 spdlog 链接 |
| `src/common/logger.hpp` | 重写 — 旧的 Logger 类替换为 LOG_* 宏 |
| `src/common/logger.cpp` | 重写 — 简化为占位文件 |
| `src/log/log_manager.h` | 修改 — 添加详细中文注释，修复析构问题 |
| `src/log/log_manager.cpp` | 修改 — 添加详细中文注释，修复 <iostream> 包含顺序 |
| `src/main/main.cpp` | 修改 — 替换日志调用，更新初始化流程 |
| `src/core/script/luau_engine.hpp` | 修改 — 新增 `register_log_api()` 声明 |
| `src/core/script/luau_engine.cpp` | 修改 — 替换日志调用，实现 `register_log_api()` |
| `src/core/agent_loop.cpp` | 修改 — 替换日志调用（printf → fmt 格式） |
| `src/core/tool_registry.cpp` | 修改 — 替换日志调用（printf → fmt 格式） |
| `src/core/api/anthropic_client.cpp` | 修改 — 替换日志调用（printf → fmt 格式） |
| `src/core/api/http_client.cpp` | 修改 — 替换日志调用（printf → fmt 格式） |
| `tests/test_common/test_logger.cpp` | 重写 — 针对新 LogManager 的测试 |
| `log_config.json` | 新增 — 日志系统配置文件 |
| `changelog/FIXES_7_CHANGELOG.md` | 新增 |
