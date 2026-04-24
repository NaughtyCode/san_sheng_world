#include <gtest/gtest.h>
#include "logger.hpp"

using namespace agent;

TEST(LoggerTest, Singleton) {
    Logger& a = Logger::instance();
    Logger& b = Logger::instance();
    EXPECT_EQ(&a, &b);
}

TEST(LoggerTest, SetLevel) {
    auto& log = Logger::instance();
    log.set_level(LogLevel::Error);
    // Log below error level should be suppressed (no crash, no output check in unit test)
    log.debug("should be suppressed");
    log.info("should be suppressed");
    log.warning("should be suppressed");
    log.error("should appear");
    log.set_level(LogLevel::Info); // restore default
}

TEST(LoggerTest, LogFormats) {
    auto& log = Logger::instance();
    // These should not crash
    log.info("int=%d str=%s", 42, "hello");
    log.warning("float=%f", 3.14);
    log.error("test error message");
}
