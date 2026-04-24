C++ AI Agent 工程文档（最终版）
1. 项目概述
实现一个基于 Anthropic API 的最小可行 AI Agent，支持工具调用和 Luau 脚本扩展。采用 C++17 标准，CMake 构建，包含完整的单元测试与自动化流程。
2. 只构建windows平台，使用Visual Studio 2026，安装目录：D:\Program Files\Microsoft Visual Studio\2022\Enterprise

2. 架构设计
2.1 整体架构图

text
┌─────────────────────────────────────────────────┐
│                 入口层（main）                    │
├─────────────────────────────────────────────────┤
│                Agent 核心循环                     │
│    ┌─────────┐ ┌─────────┐ ┌─────────────┐      │
│    │ 对话管理 │ │ 工具调用 │ │ 脚本执行引擎 │      │
│    └─────────┘ └─────────┘ └─────────────┘      │
├─────────────────────────────────────────────────┤
│               API 客户端层（Anthropic）           │
├─────────────────────────────────────────────────┤
│                   公共基础层                      │
│    ┌─────────┐ ┌─────────┐ ┌─────────────┐      │
│    │ 日志    │ │ 配置    │ │ JSON 工具   │      │
│    └─────────┘ └─────────┘ └─────────────┘      │
├─────────────────────────────────────────────────┤
│                   第三方依赖                      │
│  cpp-httplib │ nlohmann/json │ Luau            │
└─────────────────────────────────────────────────┘

2.2 核心流程
接收用户输入

构造 Anthropic Messages API 请求（含可用工具定义）

解析响应：若包含 tool_use，执行工具；否则输出回复

工具执行结果以 tool_result 角色追加到对话历史

循环直至模型返回纯文本或无工具调用

3. 模块拆分与 API 设计
3.1 公共基础层（src/common）
模块	职责	关键 API
Logger	分级日志输出	log(Level, fmt, ...), set_level(Level)
Config	环境变量/JSON 配置	load_from_file(path), get<T>(key)
Utils	字符串/时间工具	trim(), now()
3.2 API 客户端层（src/core/api）
模块	职责	关键 API
HttpClient	HTTP 封装	post(url, body, headers)
AnthropicClient	Messages API 调用	messages_create(model, msgs, tools), set_api_key(key)
3.3 Agent 核心层（src/core）
模块	职责	关键 API
Conversation	对话历史管理	add_user_msg(), add_tool_result(), get_messages()
ToolRegistry	工具注册与调用	register(name, handler), call(name, args), get_definitions()
AgentLoop	核心循环控制	run(user_input)
3.4 脚本执行层（src/core/script）
模块	职责	关键 API
LuauEngine	Luau 虚拟机，沙箱隔离	init(), load_script(path), call_function(name, args), register_cpp_function()
沙箱说明：Luau 沙箱通过创建安全环境实现，移除 os、io、debug 等危险全局函数，仅开放必要 API。

4. 工程目录结构
text
ai_agent/
├── CMakeLists.txt
├── README.md
├── scripts/
│   ├── build.sh
│   ├── test.sh
│   └── run.sh
├── external/
│   ├── CMakeLists.txt
│   ├── cpp-httplib/
│   ├── json/
│   └── luau/
├── src/
│   ├── CMakeLists.txt
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   └── main.cpp
│   ├── common/
│   │   ├── CMakeLists.txt
│   │   ├── logger.hpp / .cpp
│   │   ├── config.hpp / .cpp
│   │   └── utils.hpp / .cpp
│   └── core/
│       ├── CMakeLists.txt
│       ├── api/
│       │   ├── http_client.hpp / .cpp
│       │   └── anthropic_client.hpp / .cpp
│       ├── conversation.hpp / .cpp
│       ├── tool_registry.hpp / .cpp
│       ├── agent_loop.hpp / .cpp
│       └── script/
│           ├── luau_engine.hpp / .cpp
│           └── script_tool.hpp / .cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_common/
│   │   ├── test_logger.cpp
│   │   └── test_config.cpp
│   ├── test_core/
│   │   ├── test_conversation.cpp
│   │   ├── test_tool_registry.cpp
│   │   └── test_agent_loop.cpp
│   └── test_script/
│       └── test_luau_engine.cpp
├── scripts_luau/
│   └── example_tool.luau
└── docs/
    ├── architecture.md
    └── api_reference.md

5. 第三方库下载与部署
5.1 获取方式（Git Submodule）
bash
git submodule add https://github.com/yhirose/cpp-httplib.git external/cpp-httplib
git submodule add https://github.com/nlohmann/json.git external/json
git submodule add https://github.com/luau-lang/luau.git external/luau
git submodule update --init --recursive
5.2 CMake 集成（external/CMakeLists.txt）
cmake
# cpp-httplib (Header-only)
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib)

# nlohmann/json (Header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/include)

# Luau
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/luau EXCLUDE_FROM_ALL)
5.3 链接关系
库	类型	用途	链接目标（具体库名）
cpp-httplib	Header-only	HTTP 请求	httplib (INTERFACE)
nlohmann/json	Header-only	JSON 处理	nlohmann_json (INTERFACE)
Luau	静态库	Lua 脚本执行	Luau.VM、Luau.Compiler
6. 开发需求（依赖顺序）
Phase 0：项目骨架
0.1 创建目录结构，编写根 CMakeLists.txt，启用 C++17

0.2 配置三方库（Submodule + CMake 集成）

0.3 验证骨架构建成功

Phase 1：公共基础层
1.1 实现 Logger（分级，文件/控制台输出）

1.2 实现 Config（JSON + 环境变量）

1.3 实现 Utils

1.4 编写 Logger 和 Config 单元测试

Phase 2：API 客户端层
2.1 实现 HttpClient（封装 cpp-httplib）

2.2 实现 AnthropicClient（请求构造、响应解析）

2.3 编写 AnthropicClient 单元测试（Mock 响应）

Phase 3：Agent 核心基础
3.1 实现 Conversation（消息列表管理）

3.2 实现 ToolRegistry（注册、查找、定义生成）

3.3 编写对应单元测试

Phase 4：Luau 脚本集成
4.1 编译 Luau 静态库

4.2 实现 LuauEngine（沙箱、加载、调用、C++函数注册）

4.3 实现 ScriptTool 适配器

4.4 编写 LuauEngine 单元测试

Phase 5：Agent 循环
5.1 实现 AgentLoop（请求→响应→工具执行→结果提交 循环）

5.2 集成内置工具（计算器、脚本执行工具）

5.3 编写 AgentLoop 单元测试

Phase 6：入口与集成
6.1 编写 main.cpp 组装依赖，启动交互

6.2 编写启动脚本 scripts/run.sh

6.3 端到端集成测试

Phase 7：自动化测试
7.1 集成 Google Test

7.2 编写 test.sh（构建→测试→报告）

7.3 配置 GitHub Actions 实现自动测试与问题发现

7. 构建与测试脚本
7.1 build.sh
bash
#!/bin/bash
BUILD_TYPE=${BUILD_TYPE:-Release}
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make -j$(nproc)
7.2 test.sh
bash
#!/bin/bash
./scripts/build.sh
cd build
ctest --output-on-failure
7.3 run.sh
bash
#!/bin/bash
export ANTHROPIC_API_KEY="${ANTHROPIC_API_KEY:-}"
./build/src/main/ai_agent "$@"
8. 任务追踪约定
每完成一步需在对应位置记录完成状态，下次启动前检查，避免重复执行。建议使用以下标记格式：

markdown
- [ ] Phase 0.1 创建目录结构
- [x] Phase 0.2 配置三方库
或采用独立的 progress.md 文件维护。

9. 注意事项
代码注释需严格覆盖所有公开接口与关键逻辑

每次阶段性完成必须执行构建与测试，确保无回归

工具定义必须符合 Anthropic API 格式：name、description、input_schema

Luau 沙箱需限制文件/网络访问，仅开放必要 API

附录：修正记录
修正项	修正内容
Luau 链接库名	明确链接 Luau.VM 和 Luau.Compiler 而非笼统的 luau
构建脚本灵活性	build.sh 支持 BUILD_TYPE 环境变量，默认 Release
LuauEngine 沙箱说明	补充沙箱实现方式说明，移除危险全局函数
第三方库链接关系表	更新为实际链接目标名称
