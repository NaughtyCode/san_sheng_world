# Fixes Issue #9 — 修复控制台中文输入/输出 + 输入阻塞问题

## 概述

修复基于 Boost.Nowide + cpp-linenoise 的控制台模块 **ConsoleKit** 中三个关键 Bug：
1. **控制台无法输入**：`readline()` 中 nowide::cout 与 linenoise 冲突
2. **无法输入中文**：`win32read` 使用 AsciiChar 丢弃非 ASCII 字符
3. **中文显示乱码**：`win32_write` / `ansi::ParseAndPrintANSIString` 逐字节强转 WCHAR 拆散 UTF-8 序列

## 修改文件

### 1. `src/common/console_kit.cpp` — 移除冲突的 nowide::cout 输出

**根因**：`readline()` 在调用 `linenoise::Readline()` 之前，先通过 `boost::nowide::cout << prompt` 输出提示符。nowide 内部的 `WriteConsoleW` 操作会干扰 linenoise 随后调用的 `enableRawMode()`（同样操作 `STD_OUTPUT_HANDLE`），导致 stdin 无法切换到原始模式，用户按键无法被 `ReadConsoleInput` 捕获。

**修复**：删除 `readline()` 中的 `boost::nowide::cout << prompt; boost::nowide::cout.flush();` 两条语句。提示符由 `linenoise::Readline(prompt.c_str(), line)` 内部自行输出，无需 ConsoleKit 额外处理。

### 2. `external/cpp-linenoise/linenoise.hpp` — 四处修改

#### 2a. `win32read` 函数重写（中文输入）

**根因**：原始代码 `*c = b.Event.KeyEvent.uChar.AsciiChar` 对非 ASCII Unicode 字符返回 0（AsciiChar 字段为 0），随后 `if (*c) return 1;` 永不触发，导致函数无限循环丢弃中文按键。

**修复**：函数签名从 `int win32read(int *c)` 改为 `int win32read(char* cbuf)`，支持返回多字节 UTF-8 序列：

- Ctrl+Key 和特殊键（方向键、退格等）：继续使用 `AsciiChar`，写入 `cbuf[0]`，返回 1
- 可打印非 ASCII 字符：当 `UnicodeChar != 0` 时，通过 `WideCharToMultiByte(CP_UTF8, ...)` 将 UTF-16 宽字符转换为 1-4 字节 UTF-8 序列写入 `cbuf`，返回字节数
- 纯 ASCII 可打印字符：当 `AsciiChar != 0` 且 `UnicodeChar == 0` 时，写入 `cbuf[0]`，返回 1

#### 2b. `enableRawMode` — 启用 VT 终端处理（中文显示前提）

**修复**：在 Windows 分支的输出控制台初始化中：

1. 设置 `consolemodeOut |= ENABLE_VIRTUAL_TERMINAL_PROCESSING`，使 Windows 10 1607+ 控制台原生解释 ANSI/VT100 转义序列（光标移动、清屏、颜色等），绕过原 `ansi::ParseAndPrintANSIString` 的手工 ANSI 解析
2. 调用 `SetConsoleOutputCP(CP_UTF8)` 设置控制台输出代码页为 UTF-8
3. 若 VT 处理启用失败（旧版 Windows），回退到 `ansi::ParseAndPrintANSIString` 路径

#### 2c. `win32_write` 函数重写（中文显示）

**根因**：原始代码将 UTF-8 字节流传递给 `ansi::ParseAndPrintANSIString()`，该函数逐字节调用 `PushBuffer(*s)` — 将单个 `char` 强制转换为 `WCHAR`。例如中文 "中" (UTF-8: `E4 B8 AD`) 被拆成三个无关 Unicode 码点 `U+00E4` `U+00B8` `U+00AD`，显示为乱码。

**修复**：用 `MultiByteToWideChar(CP_UTF8, ...)` + `WriteConsoleW()` 替换 `ansi::ParseAndPrintANSIString()`：

- 将整个 UTF-8 输出缓冲区一次性转换为 UTF-16 宽字符串
- 通过 `WriteConsoleW` 写入控制台
- 配合 2b 启用的 `ENABLE_VIRTUAL_TERMINAL_PROCESSING`，ANSI 转义序列由 Windows 控制台原生解释
- 栈分配 4096 WCHAR 缓冲区（覆盖行编辑器所有单次输出），超大输出时回退到堆分配

#### 2d. 两处 `win32read` 调用点更新

**`Edit()` 调用点**（`c` 为 `int c` 值类型）：
```cpp
// 旧：nread = win32read(&c); if (nread == 1) { cbuf[0] = c; }
// 新：nread = win32read(cbuf); if (nread >= 1) { c = (unsigned char)cbuf[0]; }
```

**`completeLine()` 调用点**（`c` 为 `int *c` 指针类型）：
```cpp
// 旧：nread = win32read(c); if (nread == 1) { cbuf[0] = *c; }
// 新：nread = win32read(cbuf); if (nread >= 1) { *c = (unsigned char)cbuf[0]; }
```

## 边界处理

| 场景 | 处理方式 |
|------|---------|
| 非 tty 环境（管道/重定向） | linenoise 自动降级为 `getc(stdin)`，UTF-8 字节流透传 |
| 中文输入 | `UnicodeChar` → `WideCharToMultiByte(CP_UTF8)` → 1-4 字节 |
| 中文输出 | `MultiByteToWideChar(CP_UTF8)` → `WriteConsoleW`，完整 UTF-16 码点 |
| VT 处理不可用（Win10 1607-） | `SetConsoleMode` 失败时回退，`consolemodeOut` 保留原标志 |
| Ctrl+C / Ctrl+D | `win32read` 返回 `cbuf[0] = 3` / `cbuf[0] = 4`，linenoise 上层处理 |
| 超大输出（>4096 字节） | `win32_write` 回退到堆分配 `new WCHAR[]` |

## 验证结果

### 编译
```
零错误、零警告
全部 13 个目标编译成功
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

**非交互模式（UTF-8 管道输入）**：
```
$ echo "你好" | ./ai_agent.exe --no-interactive
→ 正常输出 API 中文响应，无乱码
```

**交互模式（管道输入）**：
```
$ printf "你好\r\nexit\r\n" | ./ai_agent.exe
→ 你好！😊 很高兴见到你！
→ 完整中文对话轮转，输入输出均无乱码
```

## 技术要点

1. **WriteConsoleW 是 Windows 下唯一可靠的 UTF-8 输出方式**：`WriteConsoleA` 在 CP_UTF8 下有历史 bug（缓冲区截断），`WriteConsoleW` 直接接受 UTF-16 绕过代码页转换
2. **Windows 10 1607+ 的 VT 处理**：设置 `ENABLE_VIRTUAL_TERMINAL_PROCESSING` 后，控制台原生处理 ANSI 转义序列，无需手工解析
3. **linenoise 的原始模式依赖**：在 `ReadConsoleInput` 读取按键之前，必须通过 `SetConsoleMode(hIn, ...)` 关闭 `ENABLE_PROCESSED_INPUT`。任何对同一控制台句柄的干扰操作（特别是输出句柄的 mode 切换）都可能破坏原始模式
