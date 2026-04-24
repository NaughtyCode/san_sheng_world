#include "logger.hpp"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <iostream>

namespace agent {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

void Logger::set_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.close();
    file_.open(path, std::ios::out | std::ios::app);
    use_file_ = file_.is_open();
}

void Logger::log(LogLevel level, const char* fmt, ...) {
    if (level < level_) return;

    char buf[4096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    write(level, buf);
}

void Logger::debug(const char* fmt, ...) {
    if (LogLevel::Debug < level_) return;
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write(LogLevel::Debug, buf);
}

void Logger::info(const char* fmt, ...) {
    if (LogLevel::Info < level_) return;
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write(LogLevel::Info, buf);
}

void Logger::warning(const char* fmt, ...) {
    if (LogLevel::Warning < level_) return;
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write(LogLevel::Warning, buf);
}

void Logger::error(const char* fmt, ...) {
    if (LogLevel::Error < level_) return;
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    write(LogLevel::Error, buf);
}

void Logger::write(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* labels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    const char* label = labels[static_cast<int>(level)];

    auto t = std::time(nullptr);
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    std::string line = std::string("[") + time_buf + "] [" + label + "] " + msg + "\n";

    if (use_file_ && file_.is_open()) {
        file_ << line;
        file_.flush();
    } else {
        std::cerr << line;
    }
}

} // namespace agent
