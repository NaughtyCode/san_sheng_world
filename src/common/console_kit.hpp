#pragma once

#include <string>
#include <functional>
#include <vector>

// Forward-declare linenoise types to avoid exposing the full header here.
// Users who need completion should include <linenoise.hpp> themselves.
namespace linenoise {
    using CompletionCallback = std::function<void(const char*, std::vector<std::string>&)>;
}

namespace agent {

// =============================================================================
// ConsoleKit — 跨平台 UTF-8 控制台输入模块
// =============================================================================
// 基于 Boost.Nowide + cpp-linenoise 封装，提供：
//   - 自动 UTF-8 编码转换（Windows 下通过 WriteConsoleW / ReadConsoleW）
//   - 行编辑：光标移动、插入、删除、历史记录翻阅
//   - 多行输入：Enter 换行，连续两次 Enter 提交
//   - 历史持久化：自动加载与保存、去重
//   - Tab 自动补全钩子
//   - 非 tty 环境（管道/重定向）自动降级为 std::getline
// =============================================================================

class ConsoleKit {
public:
    // -------------------------------------------------------------------------
    // 初始化：设置 UTF-8 控制台模式、加载历史、启用多行模式
    // @param history_path 历史记录文件路径（默认为 "history.txt"）
    // -------------------------------------------------------------------------
    static void init(const std::string& history_path = "history.txt");

    // -------------------------------------------------------------------------
    // 读取一行用户输入
    // @param prompt  显示的提示符（如 "> " 或 "agent> "）
    // @param line    [out] 用户输入的文本（不含换行符）
    // @param quit    [out] true 表示用户发出退出信号（EOF / Ctrl+D 在空行）
    // @return        true 表示成功读取，false 表示发生无法恢复的错误
    //
    // 多行模式下：
    //   - 单次 Enter 产生换行，连续两次 Enter（即空行）提交输入
    //   - 输入 "exit"、"quit" 等命令同样退出（由调用方通过 is_quit() 判断）
    //
    // 非 tty 环境（管道/文件重定向）：
    //   - 自动降级为 std::getline，无行编辑能力
    //   - 多行模式在降级模式下无效
    // -------------------------------------------------------------------------
    static bool readline(const std::string& prompt, std::string& line, bool& quit);

    // -------------------------------------------------------------------------
    // 读取一行（简化版），quit 通过返回值表达
    // @return true 表示 quit（EOF），false 表示正常输入
    // -------------------------------------------------------------------------
    static bool readline(const std::string& prompt, std::string& line);

    // -------------------------------------------------------------------------
    // 将一行文本加入历史记录（自动去重，自动裁剪超限条目）
    // -------------------------------------------------------------------------
    static void add_history(const std::string& line);

    // -------------------------------------------------------------------------
    // 保存历史记录到 init() 时指定的文件，出错时静默忽略
    // -------------------------------------------------------------------------
    static void save_history();

    // -------------------------------------------------------------------------
    // 启用/禁用多行输入模式
    // 多行模式：Enter 换行，空行（连续两次 Enter）提交
    // 单行模式：Enter 直接提交
    // -------------------------------------------------------------------------
    static void set_multiline(bool enabled);

    // -------------------------------------------------------------------------
    // 设置 Tab 自动补全回调
    // @param callback 签名: void(const char* input, std::vector<std::string>& completions)
    // -------------------------------------------------------------------------
    static void set_completion_callback(linenoise::CompletionCallback callback);

    // -------------------------------------------------------------------------
    // 判断输入是否为退出命令（"exit" / "quit" / "q"，不区分大小写）
    // 调用方在读行后应首先调用此方法检查是否需要退出 REPL
    // -------------------------------------------------------------------------
    static bool is_quit_command(const std::string& line);

    // -------------------------------------------------------------------------
    // 清理：保存历史、恢复终端模式
    // 应在程序退出前调用
    // -------------------------------------------------------------------------
    static void shutdown();

private:
    static bool initialized_;
    static bool multiline_enabled_;
    static std::string history_path_;

    // 非 tty 降级模式：读取一行原始输入
    static bool readline_fallback(const std::string& prompt, std::string& line, bool& quit);
};

} // namespace agent
