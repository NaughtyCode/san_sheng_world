#include <gtest/gtest.h>
#include "config.hpp"

using namespace agent;

TEST(ConfigTest, Defaults) {
    Config cfg;
    EXPECT_FALSE(cfg.has("nonexistent"));
    EXPECT_EQ(cfg.get<std::string>("nonexistent", "default"), "default");
    EXPECT_EQ(cfg.get<int>("nonexistent", 42), 42);
}

TEST(ConfigTest, SetAndGet) {
    Config cfg;
    cfg.set("key1", "value1");
    EXPECT_TRUE(cfg.has("key1"));
    EXPECT_EQ(cfg.get<std::string>("key1"), "value1");

    cfg.set("key2", "42");
    EXPECT_EQ(cfg.get<int>("key2"), 42);
}

TEST(ConfigTest, TypeFallback) {
    Config cfg;
    cfg.set("num", "not_a_number");
    // Should return default on type mismatch
    EXPECT_EQ(cfg.get<int>("num", 99), 99);
}
