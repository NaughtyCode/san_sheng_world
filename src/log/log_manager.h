#pragma once

// ============================================================================
// LogManager — 基于 spdlog 的统一日志管理器
//
// 功能：
//   - 单例模式，全局唯一实例
//   - 支持 JSON 配置文件加载（日志文件路径、轮转大小、最大文件数、全局级别）
//   - 支持环境变量覆盖配置（APP_LOG_FILE）
//   - 同时输出到控制台（彩色）和文件（按大小轮转）
//   - 支持按模块创建独立的 logger（如 database 模块）
//   - 异步日志，后台线程写入，不阻塞主线程
//   - 运行时动态修改任意 logger 的日志级别
//   - 优雅关闭，刷新所有未写入的日志
//
// 用法：
//   LogManager::instance().init("log_config.json");
//   auto logger = LogManager::instance().get_logger();
//   logger->info("服务器已启动, 端口: {}", port);
//   LogManager::instance().shutdown();
// ============================================================================

#include <memory>
#include <string>
#include <unordered_map>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

class LogManager {
public:
    // ---------- 单例 ----------

    /// 获取全局唯一实例（Meyer's Singleton，线程安全）
    static LogManager& instance();

    // ---------- 初始化 ----------

    /**
     * @brief 初始化日志系统，加载配置并创建 logger。
     *
     * 如果已经初始化过，则此调用无操作（幂等）。
     * 初始化时创建全局异步线程池（队列长度 8192，1 个后台线程），
     * 以及默认 logger（控制台 + 文件）和 database logger（仅文件）。
     *
     * @param config_path JSON 配置文件路径，默认为 "config.json"
     *                    若文件不存在或格式错误，则使用默认值
     * @param log_file_env_override 环境变量名，用于覆盖日志文件路径
     *                               优先级高于配置文件
     */
    void init(const std::string& config_path = "config.json",
              const std::string& log_file_env_override = "");

    // ---------- Logger 管理 ----------

    /**
     * @brief 获取指定名称的 logger。
     *
     * 如果 logger 不存在，返回默认 logger（"default"）。
     * 如果尚未初始化，则自动以默认配置初始化。
     *
     * @param name logger 名称（如 "default", "database"）
     * @return shared_ptr<spdlog::logger> 对应 logger 实例
     */
    std::shared_ptr<spdlog::logger> get_logger(const std::string& name = "default");

    /**
     * @brief 运行时动态修改指定 logger 的日志级别。
     *
     * @param name logger 名称
     * @param lvl  新的日志级别（trace/debug/info/warn/error/critical/off）
     */
    void set_level(const std::string& name, spdlog::level::level_enum lvl);

    /**
     * @brief 优雅关闭日志系统。
     *
     * 刷新所有 logger 的缓冲区，等待异步队列清空，
     * 释放 spdlog 全局资源。通常在 main() 结束时调用。
     */
    void shutdown();

private:
    LogManager() = default;
    // 析构时不调用 shutdown()，避免与 spdlog 全局线程池的析构顺序冲突。
    // spdlog 自身注册了 atexit 处理函数，会在程序退出时自动清理。
    // 如需主动关闭日志系统，请在 main() 结束前显式调用 shutdown()。
    ~LogManager() = default;

    /**
     * @brief 从 JSON 文件加载日志配置。
     *
     * 支持配置项：
     *   - log_file.path: 日志文件路径
     *   - log_file.rotation_size_mb: 单个日志文件最大大小（MB）
     *   - log_file.rotation_files: 保留的历史日志文件数
     *   - global_level: 全局日志级别字符串（trace/debug/info/warn/error/critical/off）
     *
     * @param config_path JSON 配置文件路径
     * @param env_override 环境变量名，可覆盖日志文件路径
     */
    void load_config(const std::string& config_path,
                     const std::string& env_override);

    /**
     * @brief 创建一个按大小轮转的文件 sink。
     *
     * @param path 日志文件路径
     * @param max_size_mb 单文件最大大小（MB）
     * @param max_files 保留的历史文件数
     * @return shared_ptr<sink> 文件 sink
     */
    std::shared_ptr<spdlog::sinks::sink> create_file_sink(
        const std::string& path, int max_size_mb, int max_files);

    /**
     * @brief 创建并注册一个 logger。
     *
     * logger 使用异步模式，格式为：
     * "[YYYY-MM-DD HH:MM:SS.xxx] [LEVEL] [thread_id] [file:line] message"
     *
     * @param name logger 名称
     * @param sinks sink 列表（可包含控制台 + 文件）
     * @param level 日志级别
     */
    void create_logger(const std::string& name,
                       const std::vector<std::shared_ptr<spdlog::sinks::sink>>& sinks,
                       spdlog::level::level_enum level);

    // ---------- 状态 ----------
    bool initialized_ = false;  ///< 是否已完成初始化

    /// 存储所有已注册的 logger，key 为 logger 名称
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;

    // ---------- 默认配置 ----------
    struct DefaultConfig {
        std::string log_file = "logs/app.log";     ///< 默认日志文件路径
        int rotation_size_mb = 50;                 ///< 默认单文件大小限制（MB）
        int rotation_files = 5;                    ///< 默认保留历史文件数
        std::string global_level = "info";         ///< 默认全局日志级别字符串
        spdlog::level::level_enum spdlog_level     ///< 对应的 spdlog 级别枚举
            = spdlog::level::info;
    } defaults_;
};
