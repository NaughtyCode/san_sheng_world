// ============================================================================
// LogManager 实现 — 基于 spdlog 的异步日志管理器
//
// 特性：
//   - 异步日志：后台线程写入，避免阻塞主线程
//   - 控制台彩色输出：开发调试时易于区分日志级别
//   - 文件轮转：日志文件按大小自动轮转，防止磁盘占满
//   - JSON 配置：支持通过配置文件定制日志行为
//   - 环境变量覆盖：支持 APP_LOG_FILE 环境变量动态指定日志路径
// ============================================================================

#include "log_manager.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

// 使用 nlohmann/json 进行轻量 JSON 解析（单头文件库）
// 注意：必须将 <iostream> 放在 nlohmann/json.hpp 之前，
// 因为 json.hpp 内部会声明 namespace std，可能覆盖标准库的声明
#include "nlohmann/json.hpp"
using json = nlohmann::json;

// ============================================================================
// LogManager::instance() — 全局单例
// ============================================================================
LogManager& LogManager::instance() {
    // Meyer's Singleton：C++11 保证静态局部变量初始化的线程安全
    static LogManager mgr;
    return mgr;
}

// ============================================================================
// LogManager::init() — 初始化日志系统
// ============================================================================
void LogManager::init(const std::string& config_path,
                      const std::string& log_file_env_override) {
    // 幂等：如果已经初始化过，直接返回
    if (initialized_) {
        return;
    }

    // 初始化全局异步线程池
    // 队列长度 8192：缓冲大量日志消息，减少线程切换开销
    // 1 个后台线程：足够处理普通应用的日志量，避免多线程竞争
    spdlog::init_thread_pool(8192, 1);

    // 加载配置（配置文件缺失则全部使用默认值）
    load_config(config_path, log_file_env_override);

    // ---------- 创建控制台 sink ----------
    // stdout_color_sink_mt：彩色输出到标准输出
    // 不同日志级别使用不同颜色（ERROR=红色, WARN=黄色, INFO=绿色, DEBUG=青色）
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace); // 控制台始终输出所有级别

    // ---------- 创建文件 sink ----------
    // rotating_file_sink_mt：按大小轮转的日志文件
    // 当日志文件达到指定大小后自动重命名并创建新文件
    auto file_sink = create_file_sink(defaults_.log_file,
                                      defaults_.rotation_size_mb,
                                      defaults_.rotation_files);
    file_sink->set_level(spdlog::level::trace); // 文件记录所有级别的日志

    // ---------- 创建默认 logger ----------
    // 默认 logger 同时输出到控制台和文件，适合大多数模块使用
    std::vector<spdlog::sink_ptr> default_sinks = { console_sink, file_sink };
    create_logger("default", default_sinks, defaults_.spdlog_level);

    // ---------- 创建 database 专用 logger ----------
    // database logger 仅输出到独立文件（db.log），默认 WARN 级别
    // 这是一个按模块分离日志的示例，实际项目中可按需扩展
    auto db_file_sink = create_file_sink("logs/db.log",
                                         defaults_.rotation_size_mb,
                                         defaults_.rotation_files);
    std::vector<spdlog::sink_ptr> db_sinks = { db_file_sink };
    create_logger("database", db_sinks, spdlog::level::warn);

    // 将 "default" logger 设为全局默认，这样 spdlog::info(...) 等可直接使用
    spdlog::set_default_logger(loggers_["default"]);
    spdlog::set_level(defaults_.spdlog_level);

    initialized_ = true;
}

// ============================================================================
// LogManager::get_logger() — 获取指定名称的 logger
// ============================================================================
std::shared_ptr<spdlog::logger> LogManager::get_logger(const std::string& name) {
    // 若尚未初始化，立刻以默认配置初始化（懒初始化）
    if (!initialized_) {
        init();
    }
    auto it = loggers_.find(name);
    // 找不到指定 logger 时返回默认 logger，提供容错能力
    return it != loggers_.end() ? it->second : loggers_["default"];
}

// ============================================================================
// LogManager::set_level() — 运行时修改日志级别
// ============================================================================
void LogManager::set_level(const std::string& name,
                            spdlog::level::level_enum lvl) {
    auto logger = get_logger(name);
    if (logger) {
        logger->set_level(lvl);
    }
}

// ============================================================================
// LogManager::shutdown() — 优雅关闭
// ============================================================================
void LogManager::shutdown() {
    if (!initialized_) return;
    // 刷新所有 logger 的缓冲区，确保日志不丢失
    for (auto& [_, logger] : loggers_) {
        logger->flush();
    }
    // 释放 spdlog 全局资源（关闭线程池等）
    spdlog::shutdown();
    initialized_ = false;
}

// ============================================================================
// 内部辅助函数
// ============================================================================

// ---------- load_config — 从 JSON 文件加载配置 ----------
void LogManager::load_config(const std::string& config_path,
                              const std::string& env_override) {
    // 打开配置文件
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        // 配置文件不存在，完全使用默认配置
        // 检查通过环境变量名指定的覆盖：env_override 是环境变量名而非值
        if (!env_override.empty()) {
            const char* env_val = std::getenv(env_override.c_str());
            if (env_val && env_val[0] != '\0') {
                defaults_.log_file = env_val;
            }
        }
        return;
    }

    try {
        json config = json::parse(ifs);

        // 解析日志文件配置
        if (config.contains("log_file") && config["log_file"].is_object()) {
            auto& lf = config["log_file"];
            if (lf.contains("path")) {
                defaults_.log_file = lf["path"].get<std::string>();
            }
            if (lf.contains("rotation_size_mb")) {
                defaults_.rotation_size_mb = lf["rotation_size_mb"].get<int>();
            }
            if (lf.contains("rotation_files")) {
                defaults_.rotation_files = lf["rotation_files"].get<int>();
            }
        }

        // 解析全局日志级别
        if (config.contains("global_level")) {
            std::string lvl_str = config["global_level"].get<std::string>();
            defaults_.spdlog_level = spdlog::level::from_str(lvl_str);
        }

        // 环境变量拥有最高优先级（覆盖配置文件中的值）
        // env_override 是环境变量名，需通过 std::getenv 解析其值
        if (!env_override.empty()) {
            const char* env_val = std::getenv(env_override.c_str());
            if (env_val && env_val[0] != '\0') {
                defaults_.log_file = env_val;
            }
        }
    } catch (const std::exception& e) {
        // 配置文件解析失败时，使用默认值
        // 此时日志系统尚未就绪，使用 std::cerr 输出错误提示
        std::cerr << "[LogManager] Failed to parse config file: " << config_path
                  << " (" << e.what() << "). Falling back to default configuration." << std::endl;
    }
}

// ---------- create_file_sink — 创建按大小轮转的文件 sink ----------
std::shared_ptr<spdlog::sinks::sink> LogManager::create_file_sink(
    const std::string& path, int max_size_mb, int max_files) {
    // max_size_mb * 1024 * 1024 转换为字节数
    return std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        path, max_size_mb * 1024 * 1024, max_files);
}

// ---------- create_logger — 创建并注册异步 logger ----------
void LogManager::create_logger(
    const std::string& name,
    const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks,
    spdlog::level::level_enum level) {

    // 创建异步 logger
    // spdlog::thread_pool() 使用全局异步线程池
    // async_overflow_policy::block：队列满时阻塞等待（保证日志不丢失）
    auto logger = std::make_shared<spdlog::async_logger>(
        name, sinks.begin(), sinks.end(),
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    logger->set_level(level);

    // 设置日志格式：
    // [日期 时间.毫秒] [级别] [线程ID] [文件:行号] 消息内容
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");

    // 注册到 spdlog 全局注册表
    spdlog::register_logger(logger);
    // 同时保存到本地 map 以便后续查找
    loggers_[name] = logger;
}
