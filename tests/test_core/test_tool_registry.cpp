#include <gtest/gtest.h>
#include "tool_registry.hpp"

using namespace agent;

TEST(ToolRegistryTest, RegisterAndCall) {
    ToolRegistry registry;
    json schema;
    schema["type"] = "object";

    registry.register_tool("greet", "Greet someone", schema,
        [](const json& args) -> std::string {
            std::string name = args.value("name", "world");
            return "Hello, " + name + "!";
        });

    EXPECT_TRUE(registry.has("greet"));
    EXPECT_FALSE(registry.has("unknown"));

    json args;
    args["name"] = "Alice";
    EXPECT_EQ(registry.call("greet", args), "Hello, Alice!");
}

TEST(ToolRegistryTest, CallUnknown) {
    ToolRegistry registry;
    EXPECT_EQ(registry.call("unknown", json::object()), "Error: tool not found");
}

TEST(ToolRegistryTest, GetDefinitions) {
    ToolRegistry registry;
    json schema;
    schema["type"] = "object";
    schema["properties"] = json::object();
    schema["properties"]["x"] = json::object();
    schema["properties"]["x"]["type"] = "number";

    registry.register_tool("add", "Add numbers", schema,
        [](const json&) { return "0"; });

    auto defs = registry.get_definitions();
    EXPECT_EQ(defs.size(), 1u);
    EXPECT_EQ(defs[0].name, "add");
    EXPECT_EQ(defs[0].description, "Add numbers");
    EXPECT_EQ(defs[0].input_schema["type"], "object");
}

TEST(ToolRegistryTest, ExceptionInHandler) {
    ToolRegistry registry;
    json schema;
    schema["type"] = "object";

    registry.register_tool("fail", "Always fails", schema,
        [](const json&) -> std::string {
            throw std::runtime_error("intentional failure");
        });

    std::string result = registry.call("fail", json::object());
    EXPECT_NE(result.find("Error"), std::string::npos);
}
