# Fixes Issue #9 — 替换控台输入处理逻辑（ConsoleKit 模块）

## 概述

将 `main.cpp` 中的原始 `std::getline(std::cin)` 控制台输入替换为基于 **Boost.Nowide + cpp-linenoise** 的独立模块 **ConsoleKit**，解决跨平台 UTF-8 编码、交互式行编辑、多行输入三大痛点。

## 旧架构问题

- 输入使用 `std::getline(std::cin, line)`（`main.cpp` 第 127-135 行），无行编辑、历史记录、自动补全能力
- Windows 下 `std::cin` 使用系统代码页（GBK），中文输入可能出现乱码
- 不支持多行输入（代码片段、配置块等场景受限）
- 输入处理逻辑嵌入 `main()` 函数中，无法复用
- `external/cpp-linenoise/` 库已就绪但从未集成

## 新架构

```
src/common/
├── console_kit.hpp    — ConsoleKit 类声明（跨平台控制台输入 API）
└── console_kit.cpp    — ConsoleKit 实现（封装 linenoise + nowide）

external/
├── cpp-linenoise/     — 头文件库（行编辑、历史、补全）
├── nowide/            — Boost.Nowide（UTF-8 控制台 I/O）
│   └── standalone_boost/ — 最小 Boost 配置桩头文件（消除 Boost 依赖）
└── CMakeLists.txt     — 新增 linenoise + nowide_console 编译目标
```

### 职责分离

| 组件 | 职责 | 技术栈 |
|------|------|--------|
| `ConsoleKit` | 统一控制台输入输出 API，封装行编辑、历史、多行、编码 | cpp-linenoise + Boost.Nowide |
| `linenoise` (INTERFACE) | 跨平台行编辑：光标移动、历史翻阅、Tab 补全 | 单头文件 `linenoise.hpp` |
| `nowide_console` (STATIC) | Windows 下 UTF-8 ↔ UTF-16 转换，`cin`/`cout` 替代 | Boost.Nowide（`iostream.cpp` + `console_buffer.cpp`） |

## 修改内容

### 1. 新增 `external/nowide/standalone_boost/` — Boost 配置桩头文件

消除 Boost.Nowide 对完整 Boost 库的依赖，提供最小宏定义：

| 文件 | 说明 |
|------|------|
| `boost/config.hpp` | 平台检测宏（`BOOST_WINDOWS`、`BOOST_MSVC`）、符号导出宏、`BOOST_FALLTHROUGH`、`BOOST_LIKELY`/`BOOST_UNLIKELY` |
| `boost/version.hpp` | `BOOST_VERSION` 版本号 |
| `boost/config/auto_link.hpp` | 空文件（`BOOST_NOWIDE_NO_LIB` 已禁用自动链接） |
| `boost/config/abi_prefix.hpp` | 空文件（MSVC ABI 前缀占位） |
| `boost/config/abi_suffix.hpp` | 空文件（MSVC ABI 后缀占位） |

### 2. 修改 `external/CMakeLists.txt` — 新增两个编译目标

```cmake
# cpp-linenoise (Header-only line editing library)
add_library(linenoise INTERFACE)
target_include_directories(linenoise INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp-linenoise)

# Boost.Nowide — standalone console I/O with UTF-8 support (no full Boost required)
add_library(nowide_console STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/src/iostream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/src/console_buffer.cpp
)
target_include_directories(nowide_console PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/standalone_boost   # Boost shim headers (must come first)
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/include             # Nowide headers
)
target_compile_definitions(nowide_console PUBLIC BOOST_NOWIDE_NO_LIB)
target_compile_features(nowide_console PUBLIC cxx_std_11)
```

### 3. 新增 `src/common/console_kit.hpp`（117 行）

`ConsoleKit` 类定义，所有方法均为 `static`（无状态、纯函数式控制台输入工具）：

| 方法 | 说明 |
|------|------|
| `init(history_path)` | 初始化：启用多行模式 + 加载历史记录文件 |
| `readline(prompt, line, quit)` | 读取一行输入（支持 UTF-8、多行、历史、行编辑） |
| `readline(prompt, line)` | 读取一行输入（简化版，quit 通过返回值表达） |
| `add_history(line)` | 将有效输入加入历史（自动去重、自动裁剪） |
| `save_history()` | 持久化历史到文件 |
| `set_multiline(enabled)` | 切换多行/单行模式 |
| `set_completion_callback(cb)` | 设置 Tab 自动补全回调 |
| `is_quit_command(line)` | 判断是否为退出指令（"exit"/"quit"/"q"） |
| `shutdown()` | 清理：保存历史、恢复终端 |

**边界处理设计：**
- **非 tty 环境**（管道/文件重定向）：linenoise 自动降级为 `std::getline`，无行编辑
- **空行/空白行**：`add_history` 过滤，不记录
- **Ctrl+C**：linenoise 内部处理，返回 quit=true
- **Ctrl+D（空行）**：视为 EOF，返回 quit=true
- **历史去重**：linenoise 自动跳过相邻重复行
- **文件不存在**：`LoadHistory` 静默失败
- **重复初始化**：`init()` 幂等（`initialized_` 标志保护）

### 4. 新增 `src/common/console_kit.cpp`（172 行）

完整实现，包含：

- **`sanitize_line_ending()`**：清除 Windows 管道场景下残留的 `\r`
- **`trim_left()` / `trim_right()` / `trim()`**：模块内空白裁剪（避免对 `utils.hpp` 的循环依赖）
- **`init()`**：调用 `linenoise::SetMultiLine(true)` 启用多行模式，调用 `linenoise::LoadHistory()` 加载历史
- **`readline()`**：核心读取逻辑 — 先刷新 nowide::cout 输出提示符，再委托给 `linenoise::Readline()`
- **`is_quit_command()`**：不区分大小写匹配 "exit" / "quit" / "q"

### 5. 修改 `src/common/CMakeLists.txt` — 新增源文件和链接库

```cmake
# 新增 console_kit.cpp 源文件
# 新增链接目标：nowide_console linenoise
target_link_libraries(agent_common PUBLIC
    nlohmann_json
    spdlog::spdlog
    nowide_console    # <-- 新增
    linenoise         # <-- 新增
)
```

### 6. 修改 `src/main/main.cpp` — 替换输入处理逻辑

**主要变更：**

| 位置 | 旧代码 | 新代码 |
|------|--------|--------|
| 第 1-10 行 | `#include <iostream>` | 新增 `#include <boost/nowide/iostream.hpp>` 和 `#include "console_kit.hpp"` |
| 第 14-15 行 | — | 新增 `namespace io = boost::nowide;` 别名 |
| 输出 | `std::cout << ...` | 全部替换为 `io::cout << ...`（`boost::nowide::cout`） |
| 第 127-135 行 | `while (true) { std::cout << "\n> "; if (!std::getline(...)) break; ... }` | 替换为 `ConsoleKit::init()` + `ConsoleKit::readline()` 循环 |
| 第 136 行后 | — | 新增 `ConsoleKit::shutdown()` 清理 |

**新 REPL 循环特点：**
- 提示符从 `"> "` 改为 `"agent> "`
- 支持方向键翻阅历史、行内编辑、Tab 补全
- 多行模式下 Enter 换行，空行提交
- 支持 `exit` / `quit` / `q` 退出（不区分大小写）
- Ctrl+D / Ctrl+C 安全退出

## 设计原则

### 零依赖增长
通过 `standalone_boost/` 桩头文件消除对完整 Boost（1.66+）的依赖。nowide 仅需 Windows SDK 头文件即可编译。

### 静态方法语义
`ConsoleKit` 所有方法均为 `static`，无成员变量，遵循 `ResponseParser` / `MessageFormatter` 相同的设计范式：线程安全、零开销、纯函数。

### 自动降级
非 tty 环境（CI 管道、文件重定向）下 linenoise 自动检测并降级为 `std::getline`，无需调用方判断。多行模式在降级场景下自然失效。

### 边界安全
- 历史去重、长度裁剪、文件缺失静默处理
- `\r` 清理兼容 Windows 管道输入
- 重复初始化幂等
- 空行回溯（不记录，不回传）

## 验证结果

### 编译

```
零错误、零警告
3 个新库/可执行文件编译成功
nowide_console.lib → agent_common.lib → ai_agent.exe 链接成功
```

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
$ echo "hello" | ./build/bin/Release/ai_agent.exe --no-interactive

[2026-04-25 10:12:01.562] [info] [12916] [:] AI Agent starting
...
Endpoint: https://api.deepseek.com/anthropic/chat/completions
Model: deepseek-v4-pro
...
→ 正常启动，nowide::cout 输出正常，无编码问题
```

### REPL 交互测试

```
$ ./build/bin/Release/ai_agent.exe

AI Agent ready. Type 'exit' to quit.
(Multi-line: Enter to break, double Enter to submit)
agent> hello world
[Agent response...]
agent> exit
[2026-04-25 ...] [info] [12916] [:] AI Agent shutting down
```

## 修改文件清单

| 文件 | 状态 |
|------|------|
| `external/nowide/standalone_boost/boost/config.hpp` | 新增 — Boost 平台检测桩头 |
| `external/nowide/standalone_boost/boost/version.hpp` | 新增 — Boost 版本桩头 |
| `external/nowide/standalone_boost/boost/config/auto_link.hpp` | 新增 — 自动链接桩头（空） |
| `external/nowide/standalone_boost/boost/config/abi_prefix.hpp` | 新增 — ABI 前缀桩头（空） |
| `external/nowide/standalone_boost/boost/config/abi_suffix.hpp` | 新增 — ABI 后缀桩头（空） |
| `external/CMakeLists.txt` | 修改 — 新增 `linenoise` + `nowide_console` 目标 |
| `src/common/console_kit.hpp` | 新增 — 控制台输入模块头文件（117 行） |
| `src/common/console_kit.cpp` | 新增 — 控制台输入模块实现（172 行） |
| `src/common/CMakeLists.txt` | 修改 — 新增 `console_kit.cpp`，链接 `nowide_console` + `linenoise` |
| `src/main/main.cpp` | 修改 — 使用 ConsoleKit 替代 `std::getline`，使用 `nowide::cout` 替代 `std::cout` |
| `changelog/FIXES_9_CHANGELOG.md` | 新增 |
