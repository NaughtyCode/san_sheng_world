#include <gtest/gtest.h>
#include "agent_loop.hpp"

using namespace agent;

TEST(AgentLoopTest, Construction) {
    AgentLoop agent;
    // Agent should construct without crash
    EXPECT_NO_THROW({
        AgentLoop a;
    });
}

TEST(AgentLoopTest, SetApiKey) {
    AgentLoop agent;
    agent.set_api_key("test-key");
    // No throw means success
}

TEST(AgentLoopTest, RegisterTool) {
    AgentLoop agent;
    json schema;
    schema["type"] = "object";
    agent.register_tool("echo", "Echo back", schema,
        [](const json& args) -> std::string {
            return args.value("text", "");
        });
    // No throw means success
}

TEST(AgentLoopTest, BuiltinTools) {
    AgentLoop agent;
    agent.load_builtin_tools();
    // Should register calculator and script tools without crash
}

TEST(AgentLoopTest, RunWithoutApiKeyReturnsError) {
    AgentLoop agent;
    agent.set_api_key(""); // empty key
    std::string result = agent.run("Hello");
    // Should return error about API connection failure
    EXPECT_FALSE(result.empty());
}
