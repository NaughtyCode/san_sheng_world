#include "console_kit.hpp"

#include <boost/nowide/iostream.hpp>  // boost::nowide::cout — 跨平台 UTF-8 控制台输出
#include "linenoise.hpp"              // linenoise::Readline 等行编辑 API

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace agent {

// =============================================================================
// 静态成员初始化
// =============================================================================
bool        ConsoleKit::initialized_       = false;
bool        ConsoleKit::multiline_enabled_  = false;
std::string ConsoleKit::history_path_       = "history.txt";

// =============================================================================
// 防止 \r\n 或单独的 \r 残留（Windows 管道/文件重定向场景）
// =============================================================================
static std::string sanitize_line_ending(std::string s)
{
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
    return s;
}

// =============================================================================
// 简单空白裁剪（保留 utils::trim 的语义但避免循环依赖）
// =============================================================================
static std::string trim_left(std::string s)
{
    auto it = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    s.erase(s.begin(), it);
    return s;
}
static std::string trim_right(std::string s)
{
    auto it = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    s.erase(it.base(), s.end());
    return s;
}
static std::string trim(std::string s)
{
    return trim_left(trim_right(s));
}

// =============================================================================
// init
// =============================================================================
void ConsoleKit::init(const std::string& history_path)
{
    // 防止重复初始化
    if (initialized_) {
        return;
    }

    history_path_ = history_path;

    // 启用多行模式：Enter 换行，连续两次 Enter（空行）提交
    linenoise::SetMultiLine(true);
    multiline_enabled_ = true;

    // 从文件加载历史记录（文件不存在时静默失败）
    if (!history_path_.empty()) {
        linenoise::LoadHistory(history_path_.c_str());
    }

    initialized_ = true;
}

// =============================================================================
// readline — 核心读取函数
// =============================================================================
bool ConsoleKit::readline(const std::string& prompt, std::string& line, bool& quit)
{
    if (!initialized_) {
        // 未显式初始化时自动执行默认初始化
        init();
    }

    quit = false;
    line.clear();

    // 委托给 linenoise 进行行编辑
    // Readline 内部自行输出提示符，此处不应再向控制台写入任何内容，
    // 否则 nowide 的 WriteConsoleW 操作会干扰 linenoise 的 enableRawMode 调用，
    // 导致 stdin 无法正确切换到原始模式，进而阻止用户输入。
    quit = linenoise::Readline(prompt.c_str(), line);

    // 清理 Windows 下可能残留的 \r
    line = sanitize_line_ending(line);

    if (quit) {
        // 用户按 Ctrl+D 或 Ctrl+C（linenoise 视其为退出信号），
        // 已在 linenoise 内部处理终端恢复
        return true;
    }

    return true; // 成功读到了内容（内容可能为空字符串）
}

// =============================================================================
// readline — 简化版（quit 通过返回值表达）
// =============================================================================
bool ConsoleKit::readline(const std::string& prompt, std::string& line)
{
    bool quit = false;
    readline(prompt, line, quit);
    return quit;
}

// =============================================================================
// add_history
// =============================================================================
void ConsoleKit::add_history(const std::string& line)
{
    // 空行不记录，空白行不记录
    std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return;
    }

    linenoise::AddHistory(line.c_str());
}

// =============================================================================
// save_history
// =============================================================================
void ConsoleKit::save_history()
{
    if (history_path_.empty()) {
        return;
    }

    linenoise::SaveHistory(history_path_.c_str());
}

// =============================================================================
// set_multiline
// =============================================================================
void ConsoleKit::set_multiline(bool enabled)
{
    multiline_enabled_ = enabled;
    linenoise::SetMultiLine(enabled);
}

// =============================================================================
// set_completion_callback
// =============================================================================
void ConsoleKit::set_completion_callback(linenoise::CompletionCallback callback)
{
    linenoise::SetCompletionCallback(std::move(callback));
}

// =============================================================================
// is_quit_command
// =============================================================================
bool ConsoleKit::is_quit_command(const std::string& line)
{
    std::string lower = line;
    // 转小写（仅 ASCII）
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    lower = trim(lower);

    return lower == "exit" || lower == "quit" || lower == "q";
}

// =============================================================================
// shutdown
// =============================================================================
void ConsoleKit::shutdown()
{
    save_history();
    // linenoise 在 atexit 时会自动恢复终端，此处为显式调用提供入口
}

} // namespace agent
