#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace agent {

enum class LogLevel { Debug = 0, Info = 1, Warning = 2, Error = 3 };

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    void set_file(const std::string& path);

    void log(LogLevel level, const char* fmt, ...);

    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warning(const char* fmt, ...);
    void error(const char* fmt, ...);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write(LogLevel level, const std::string& msg);

    LogLevel level_ = LogLevel::Info;
    std::mutex mutex_;
    std::ofstream file_;
    bool use_file_ = false;
};

} // namespace agent
