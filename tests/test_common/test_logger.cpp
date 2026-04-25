// ==========================================================================
// 新日志系统单元测试 — 基于 spdlog + LogManager
// 测试 LogManager 单例、初始化、logger 获取、日志级别设置、宏输出等
// ==========================================================================

#include <gtest/gtest.h>
#include "logger.hpp"

// ==========================================================================
// LogManager 单例测试
// ==========================================================================
TEST(LogManagerTest, Singleton) {
    // LogManager 是 Meyer's Singleton，多次调用 instance() 应返回同一实例
    LogManager& a = LogManager::instance();
    LogManager& b = LogManager::instance();
    EXPECT_EQ(&a, &b);
}

// ==========================================================================
// LogManager 初始化测试
// ==========================================================================
TEST(LogManagerTest, InitWithDefaults) {
    // 使用默认配置初始化（不指定配置文件，使用全部默认值）
    LogManager::instance().init("", "");
    // 初始化后应能获取到默认 logger
    auto logger = LogManager::instance().get_logger();
    EXPECT_NE(logger, nullptr);
    // 初始化不应崩溃，且默认级别为 info
    EXPECT_EQ(logger->level(), spdlog::level::info);
}

// ==========================================================================
// LogManager::get_logger 测试
// ==========================================================================
TEST(LogManagerTest, GetLogger) {
    // 获取默认 logger
    auto default_logger = LogManager::instance().get_logger("default");
    EXPECT_NE(default_logger, nullptr);
    // 获取不存在的 logger 应返回默认 logger
    auto unknown_logger = LogManager::instance().get_logger("nonexistent");
    EXPECT_NE(unknown_logger, nullptr);
    // 应返回同一个默认 logger
    EXPECT_EQ(default_logger, unknown_logger);
}

// ==========================================================================
// LogManager::set_level 测试
// ==========================================================================
TEST(LogManagerTest, SetLevel) {
    auto logger = LogManager::instance().get_logger();
    // 设置为 error 级别
    LogManager::instance().set_level("default", spdlog::level::err);
    EXPECT_EQ(logger->level(), spdlog::level::err);
    // 恢复为 info 级别
    LogManager::instance().set_level("default", spdlog::level::info);
    EXPECT_EQ(logger->level(), spdlog::level::info);
}

// ==========================================================================
// 日志宏输出测试 — 验证 LOG_* 宏不崩溃
// ==========================================================================
TEST(LogManagerTest, MacroOutputs) {
    // 以下宏调用不应导致崩溃或异常
    LOG_TRACE("trace test: {}", 1);
    LOG_DEBUG("debug test: {} + {} = {}", 1, 1, 2);
    LOG_INFO("info test: string={}", "hello");
    LOG_WARN("warn test: float={:.2f}", 3.14);
    LOG_ERROR("error test: {}", "error message");
    LOG_CRITICAL("critical test: {}", "critical message");

    // 测试整数和浮点数格式化
    LOG_INFO("int={} double={} string={}", 42, 3.14, std::string("test"));

    // 无参数日志
    LOG_INFO("simple informational message");
    SUCCEED();
}

// ==========================================================================
// LogManager 关闭测试
// ==========================================================================
TEST(LogManagerTest, ShutdownAndReinit) {
    // 关闭日志系统
    LogManager::instance().shutdown();
    // 关闭后获取 logger 应触发懒初始化，不应崩溃
    auto logger = LogManager::instance().get_logger();
    EXPECT_NE(logger, nullptr);
    LOG_INFO("logger reinitialized after shutdown");
}
