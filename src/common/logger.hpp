#pragma once

// ==========================================================================
// 新日志系统头文件
// 基于 spdlog + LogManager 提供便捷的日志宏，替换旧的 agent::Logger 类。
//
// 用法:
//   LOG_INFO("服务器已启动, 端口: {}", port);
//   LOG_DEBUG("请求参数: key={}, value={}", k, v);
//   LOG_WARN("内存使用率偏高: {}%", usage);
//   LOG_ERROR("连接失败: {}", e.what());
//
// 日志同时输出到控制台（彩色）和文件（按大小轮转）。
// ==========================================================================

#include "log_manager.h"

// ---------- 便捷日志宏 ----------
// 自动获取默认 logger，使用 spdlog 的 fmt 风格格式化字符串

/// 输出 TRACE 级别日志（最详细，通常仅开发调试使用）
#define LOG_TRACE(...)    ::LogManager::instance().get_logger()->trace(__VA_ARGS__)

/// 输出 DEBUG 级别日志（调试信息）
#define LOG_DEBUG(...)    ::LogManager::instance().get_logger()->debug(__VA_ARGS__)

/// 输出 INFO 级别日志（常规运行信息）
#define LOG_INFO(...)     ::LogManager::instance().get_logger()->info(__VA_ARGS__)

/// 输出 WARN 级别日志（警告，不影响运行但需要关注）
#define LOG_WARN(...)     ::LogManager::instance().get_logger()->warn(__VA_ARGS__)

/// 输出 ERROR 级别日志（错误，但系统仍可继续运行）
#define LOG_ERROR(...)    ::LogManager::instance().get_logger()->error(__VA_ARGS__)

/// 输出 CRITICAL 级别日志（严重错误，可能导致系统崩溃）
#define LOG_CRITICAL(...) ::LogManager::instance().get_logger()->critical(__VA_ARGS__)
