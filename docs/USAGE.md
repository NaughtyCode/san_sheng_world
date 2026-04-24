# AI Agent 使用教程

## 项目概述

`ai_agent` 是一个基于 **Anthropic Messages API** 的命令行 AI 助手，使用 **C++17** 开发，CMake 构建。核心特性包括：

- **交互式对话**：支持 REPL 交互模式和一次性问答模式
- **工具调用（Tool Use）**：支持 Anthropic 原生的 tool_use 功能，AI 可自主调用本地工具
- **Luau 脚本扩展**：内置 Luau 脚本引擎，可加载自定义脚本作为新工具
- **安全沙箱**：Luau 引擎已移除危险全局函数（os, io, debug 等）

---

## 1. 环境要求

| 依赖 | 要求 |
|------|------|
| 编译器 | Visual Studio 2022 (Windows) 或 GCC/Clang 支持 C++17 |
| CMake | >= 3.16 |
| Git | 包含子模块支持 |
| API Key | Anthropic API Key（需自行申请） |

---

## 2. 快速开始

### 2.1 克隆并初始化子模块

```bash
git clone <仓库地址>
cd san_sheng_world
git submodule update --init --recursive
```

### 2.2 编译

**Windows（推荐）：**
```cmd
.\scripts\build.bat
```

脚本会自动查找 Visual Studio 2022 的 `VsDevCmd.bat` 并初始化编译环境。

**Linux / WSL：**
```bash
BUILD_TYPE=Release ./scripts/build.sh
```

编译产物位于 `build/bin/Release/`，包括：
- `ai_agent.exe` —— 主程序
- `test_*.exe` —— 各模块单元测试

### 2.3 运行

**配置 API Key（必需）：**
```cmd
set ANTHROPIC_API_KEY=sk-ant-xxxxx
```

或通过环境变量永久设置，也可写入 JSON 配置文件（见第 3 节）。

**启动交互模式：**
```cmd
.\scripts\run.bat
```

**一次性问答：**
```cmd
.\scripts\run.bat "法国的首都是哪里？"
```

**Linux：**
```bash
export ANTHROPIC_API_KEY="sk-ant-xxxxx"
./scripts/run.sh "What is the capital of France?"
```

---

## 3. 配置指南

配置可通过**环境变量**或 **JSON 配置文件**两种方式加载。

### 3.1 环境变量

| 变量名 | 用途 | 默认值 |
|--------|------|--------|
| `ANTHROPIC_API_KEY` | API 认证密钥 | 无（必需） |
| `ANTHROPIC_MODEL` | 模型名称 | `claude-sonnet-4-6` |
| `ANTHROPIC_BASE_URL` | API 基础 URL | `https://api.anthropic.com` |
| `LOG_LEVEL` | 日志级别 | `info` |
| `LOG_FILE` | 日志文件路径（空=输出到 stderr） | 空 |

### 3.2 JSON 配置文件

创建 `config.json`：
```json
{
    "api_key": "sk-ant-xxxxx",
    "model": "claude-sonnet-4-6",
    "log_level": "debug",
    "log_file": "agent.log"
}
```

在代码中调用 `config.load_from_file("config.json")` 即可加载。

### 3.3 支持的日志级别

- `debug` — 调试信息（含 API 请求详情、工具调用参数）
- `info` — 一般信息（启动、工具注册、循环迭代）
- `warning` — 警告（工具未找到、达到最大迭代次数）
- `error` — 错误（API 失败、HTTP 错误、JSON 解析错误）

---

## 4. 命令行参数

```
ai_agent.exe [选项] [提示词]
```

| 参数 | 说明 |
|------|------|
| `-n`, `--no-interactive` | 非交互模式（输出结果后退出） |
| `-m <model>`, `--model <model>` | 指定模型（覆盖 ANTHROPIC_MODEL） |
| `<任意文本>` | 初始提示词，多个参数自动拼接 |

### 示例

```cmd
REM 使用指定模型进行一次性问答
ai_agent.exe -m claude-opus-4-7 -n "解释量子纠缠"

REM 交互模式（默认）
ai_agent.exe

REM 带初始提示进入交互模式
ai_agent.exe "你好，请介绍一下自己"
```

---

## 5. 交互模式

启动后进入 REPL 循环：

```
AI Agent ready. Type 'exit' to quit.

> 帮我计算 123 * 456 的结果
[Agent 可能调用 calculator 工具，然后返回结果]

> exit
```

- 输入 `exit` 或 `quit` 退出
- 按 `Ctrl+C` 强制退出
- 直接输入问题即可，Agent 将自动调用可用工具完成任务

---

## 6. 内置工具

### 6.1 calculator — 数学计算器

调用 Luau 引擎执行数学表达式求值。

**输入参数：**
```json
{
    "expression": "2 + 3 * 4 - 8 / 2"
}
```

**使用示例：**
```
> 帮我算一下 3.14 * 15 * 15 的结果
```

Agent 会自动调用 calculator 工具，传入 `expression: "3.14 * 15 * 15"`。

### 6.2 Luau 自定义脚本工具

通过 `agent.load_script_tool(path, func_name)` 加载 `.luau` 文件中的函数，使之成为 AI 可调用的工具。

---

## 7. Luau 脚本扩展

### 7.1 内置示例脚本

示例脚本位于 `scripts_luau/example_tool.luau`，提供三个函数：

| 函数 | 说明 | 参数 |
|------|------|------|
| `eval_expr(expr)` | 安全地计算数学表达式 | `expr: string` |
| `reverse_string(s)` | 反转字符串 | `s: string` |
| `to_json_string(t)` | 将 Lua table 转换为 JSON 字符串 | `t: table` |

### 7.2 编写自定义脚本工具

在项目代码中添加自定义 `.luau` 脚本：

```lua
-- my_tools.luau — 自定义工具集

-- 天气查询工具（示例）
function get_weather(city: string): string
    -- 实际项目中这里可以调用 HTTP API
    return "城市 " .. city .. " 的天气：晴天，25°C"
end

-- 文本翻译工具
function translate(text: string, target_lang: string): string
    -- 模拟翻译
    return "[" .. target_lang .. "] " .. text
end

-- 返回需要暴露给 Agent 的函数
return {
    get_weather = get_weather,
    translate = translate,
}
```

### 7.3 在代码中加载脚本

```cpp
AgentLoop agent;
agent.set_api_key(api_key);

// 加载 Luau 脚本作为工具
agent.load_script_tool("scripts_luau/my_tools.luau", "get_weather");
agent.load_script_tool("scripts_luau/my_tools.luau", "translate");
```

### 7.4 安全注意事项

Luau 引擎已移除以下危险函数：
- `os`（操作系统接口）
- `io`（文件 IO）
- `debug`（调试接口）
- `loadfile`, `dofile`, `require`（文件加载）

脚本中的 `eval_expr` 使用 `loadstring` + `pcall` 模式安全求值，避免代码注入。

---

## 8. 项目架构

```
┌─────────────────────────────────────────────────┐
│            入口层 (src/main/)                    │
│  main.cpp: 配置初始化、Agent 创建、CLI 循环      │
├─────────────────────────────────────────────────┤
│            核心层 (src/core/)                    │
│  AgentLoop:    请求-响应-工具 主循环             │
│  Conversation: 对话历史管理                      │
│  ToolRegistry:  工具注册与调度                   │
├─────────────────────┬───────────────────────────┤
│  API 客户端层        │  脚本引擎                  │
│  HttpClient          │  LuauEngine                │
│  AnthropicClient     │  ScriptTool 适配器         │
├─────────────────────┴───────────────────────────┤
│            公共层 (src/common/)                   │
│  Logger: 分级日志 (控制台/文件)                   │
│  Config: JSON + 环境变量 配置                    │
│  Utils:  字符串/时间 工具函数                     │
├─────────────────────────────────────────────────┤
│            第三方依赖                              │
│  cpp-httplib | nlohmann/json | Luau              │
└─────────────────────────────────────────────────┘
```

### 数据流

```
用户输入 → AgentLoop.run()
  → 添加到 Conversation
  → AnthropicClient.messages_create() → HttpClient.post() → Anthropic API
  → 解析响应:
      ├─ 纯文本 → 添加到 Conversation → 返回结果
      └─ tool_use → ToolRegistry.call() → 结果加入 Conversation → 循环
```

### 循环控制

- 默认最大迭代次数：**10 次**（可通过 `set_max_iterations()` 修改）
- 默认最大 Token 数：**4096**（可通过 `set_max_tokens()` 修改）
- 停止条件：API 返回 `end_turn`、`stop_sequence`、`max_tokens`、或无 tool_use 的纯文本响应

---

## 9. API 参考

### 9.1 AgentLoop（核心调度器）

```cpp
AgentLoop agent;

// 配置 API
agent.set_api_key("sk-ant-xxxxx");
agent.set_model("claude-sonnet-4-6");
agent.set_max_tokens(4096);
agent.set_max_iterations(10);

// 工具管理
agent.register_tool("name", "desc", json_schema, handler);
agent.load_builtin_tools();              // 加载内置 calculator
agent.load_script_tool("path.luau", "func_name");  // 加载脚本工具
agent.script_engine();                   // 获取 LuauEngine 引用

// 执行
std::string result = agent.run("用户输入");
```

### 9.2 ToolRegistry（工具注册表）

```cpp
// handler: std::function<std::string(const nlohmann::json&)>
tool_registry.register_tool(name, description, input_schema, handler);
tool_registry.call(name, args);          // 调用工具
tool_registry.has(name);                 // 检查是否存在
tool_registry.get_definitions();         // 获取所有工具定义（用于 API 请求）
```

**input_schema 示例：**
```json
{
    "type": "object",
    "properties": {
        "expression": {
            "type": "string",
            "description": "数学表达式"
        }
    },
    "required": ["expression"]
}
```

### 9.3 Conversation（对话管理）

```cpp
conversation.add_user_msg("用户消息");
conversation.add_assistant_msg("回复文本", "tool_name", "tool_id", tool_input);
conversation.add_tool_result("tool_id", "结果字符串");
conversation.get_messages();     // 获取全部消息
conversation.clear();            // 重置对话
```

### 9.4 LuauEngine（脚本引擎）

```cpp
LuauEngine& luau = agent.script_engine();

luau.init();                           // 初始化沙箱环境
luau.load_script("path/to/tools.luau"); // 加载脚本文件
luau.load_string("name", "source");     // 直接执行字符串
luau.call_function("func_name", args_json); // 调用函数
luau.register_cpp_function("name", handler); // 注册 C++ 函数到 Lua
```

### 9.5 Config（配置管理）

```cpp
Config config;
config.load_from_env();               // 从环境变量加载
config.load_from_file("config.json"); // 从文件加载
config.get<std::string>("key", "default");
config.get<int>("timeout", 30);
config.set("key", "value");
config.has("key");
```

### 9.6 Logger（日志系统）

```cpp
Logger& log = Logger::instance();     // 单例
log.set_level(LogLevel::Debug);
log.set_file("agent.log");

log.debug("debug message: %d", value);
log.info("info message: %s", str);
log.warning("warning: %s", str);
log.error("error: code=%d", code);
```

### 9.7 Utils（工具函数）

```cpp
#include "utils.hpp"

std::string s = agent::utils::trim("  hello  ");      // "hello"
std::string t = agent::utils::now();                   // "2026-04-24T10:30:00Z"
std::string l = agent::utils::to_lower("HELLO");       // "hello"
bool b1 = agent::utils::starts_with("hello", "he");    // true
bool b2 = agent::utils::ends_with("world", "ld");      // true
```

---

## 10. 运行测试

```cmd
REM Windows
.\scripts\test.bat

REM Linux
./scripts/test.sh
```

测试基于 GoogleTest，自动通过 CMake `FetchContent` 下载（无需手动安装）。

测试覆盖：
| 模块 | 测试文件 | 测试内容 |
|------|----------|----------|
| Logger | `test_logger.cpp` | 日志级别过滤、文件输出、单例模式 |
| Config | `test_config.cpp` | 环境变量加载、JSON 文件加载、类型转换 |
| Conversation | `test_conversation.cpp` | 消息增删、历史管理 |
| ToolRegistry | `test_tool_registry.cpp` | 工具注册、调用、异常处理 |
| AgentLoop | `test_agent_loop.cpp` | 主循环逻辑（需要 Mock API） |
| LuauEngine | `test_luau_engine.cpp` | 脚本加载、函数调用、JSON 互转、沙箱 |

---

## 11. 开发指南

### 11.1 添加新的内置工具

在 `agent_loop.cpp` 的 `load_builtin_tools()` 中添加：

```cpp
// 注册天气工具
json weather_schema;
weather_schema["type"] = "object";
weather_schema["properties"] = json::object();
weather_schema["properties"]["city"] = json::object();
weather_schema["properties"]["city"]["type"] = "string";
weather_schema["properties"]["city"]["description"] = "城市名称";
weather_schema["required"] = json::array({"city"});

tool_registry_.register_tool(
    "weather",
    "查询指定城市的天气信息",
    weather_schema,
    [this](const json& args) -> std::string {
        std::string city = args.value("city", "");
        // 调用实际天气 API
        return "城市 " + city + " 天气：晴天";
    }
);
```

### 11.2 添加新模块到 CMake

编辑 `src/CMakeLists.txt`，参考 `agent_common` 和 `agent_core` 的模式添加新的静态库：

```cmake
add_library(my_module STATIC
    my_module/my_class.hpp
    my_module/my_class.cpp
)
target_link_libraries(my_module PUBLIC agent_common)
```

### 11.3 项目目录结构

```
san_sheng_world/
├── CMakeLists.txt                 # 根 CMake 配置
├── src/
│   ├── CMakeLists.txt             # 源文件 CMake
│   ├── common/                    # agent_common 静态库
│   │   ├── logger.hpp/.cpp
│   │   ├── config.hpp/.cpp
│   │   └── utils.hpp/.cpp
│   ├── core/                      # agent_core 静态库
│   │   ├── agent_loop.hpp/.cpp    # 主循环
│   │   ├── conversation.hpp/.cpp  # 对话历史
│   │   ├── tool_registry.hpp/.cpp # 工具注册
│   │   ├── api/
│   │   │   ├── http_client.hpp/.cpp
│   │   │   └── anthropic_client.hpp/.cpp
│   │   └── script/
│   │       ├── luau_engine.hpp/.cpp
│   │       └── script_tool.hpp/.cpp
│   └── main/
│       ├── CMakeLists.txt
│       └── main.cpp               # 入口
├── tests/                         # 单元测试
│   ├── test_common/
│   ├── test_core/
│   └── test_script/
├── scripts/                       # 构建/运行脚本
│   ├── build.sh / build.bat
│   ├── test.sh / test.bat
│   └── run.sh / run.bat
├── scripts_luau/                  # Luau 示例脚本
│   └── example_tool.luau
├── external/                      # 第三方子模块
│   ├── cpp-httplib/               # HTTP 客户端
│   ├── json/                      # JSON 解析
│   └── luau/                      # Luau 脚本引擎
└── docs/
    ├── architecture.md
    └── api_reference.md
```

---

## 12. 常见问题

### Q: 启动提示 "ANTHROPIC_API_KEY not set"

需要设置环境变量 `ANTHROPIC_API_KEY`：
```cmd
set ANTHROPIC_API_KEY=sk-ant-xxxxx
```

或在代码中修改 `main.cpp`，增加 `config.load_from_file("config.json")` 调用。

### Q: 编译时提示找不到 Visual Studio

修改 `scripts/build.bat` 中 Visual Studio 的搜索路径，或直接使用 CMake：
```cmd
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Release --parallel
```

### Q: HTTP 请求失败

检查以下几点：
1. API Key 是否正确且有效
2. 网络是否可以访问 `https://api.anthropic.com`
3. 如有代理，需设置 `ANTHROPIC_BASE_URL` 指向代理地址
4. 查看日志（设置 `LOG_LEVEL=debug`）获取详细错误信息

### Q: 工具没有被调用

确认：
1. 工具是否已注册（`tool_registry_.has("tool_name")` 返回 true）
2. input_schema 定义是否正确
3. 工具描述是否清晰（AI 根据描述判断是否调用）

### Q: Luau 脚本加载失败

常见原因：
1. 脚本文件中使用了被禁用的全局函数（os、io 等）
2. 脚本语法错误
3. 脚本未返回 table（`return { ... }`）

---

## 13. 许可证

参见项目 LICENSE 文件。第三方依赖使用各自许可证：
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) — MIT
- [nlohmann/json](https://github.com/nlohmann/json) — MIT
- [Luau](https://github.com/luau-lang/luau) — MIT
