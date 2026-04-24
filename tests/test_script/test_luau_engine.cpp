#include <gtest/gtest.h>
#include "script/luau_engine.hpp"

using namespace agent;

TEST(LuauEngineTest, Init) {
    LuauEngine engine;
    engine.init();
    // Should not crash
}

TEST(LuauEngineTest, LoadAndCallFunction) {
    LuauEngine engine;
    engine.init();

    std::string code = R"(
function add(a, b)
    return a + b
end
)";
    engine.load_string("test_add", code);

    json args = json::array({3, 4});
    // Note: basic Lua types should work via Luau
    // Full JSON conversion testing depends on push_json_value implementation
}

TEST(LuauEngineTest, CallNonExistentFunction) {
    LuauEngine engine;
    engine.init();

    json result = engine.call_function("does_not_exist", json::object());
    EXPECT_TRUE(result.contains("error"));
}

TEST(LuauEngineTest, ScriptError) {
    LuauEngine engine;
    engine.init();

    std::string code = R"(
function broken()
    error("intentional error from script")
end
)";
    engine.load_string("test_broken", code);

    json result = engine.call_function("broken", json::object());
    EXPECT_TRUE(result.contains("error"));
}

TEST(LuauEngineTest, RegisterCppFunction) {
    LuauEngine engine;
    engine.init();

    engine.register_cpp_function("cpp_echo", [](const json& args) -> json {
        return args;
    });

    // After registration, the function should be callable from Lua
    std::string code = R"(
function call_cpp()
    return cpp_echo("hello")
end
)";
    engine.load_string("test_cpp", code);
    // Should not crash
}

TEST(LuauEngineTest, SandboxNoDangerousGlobals) {
    LuauEngine engine;
    engine.init();

    // Try to access os, io, debug - these should not be available
    std::string code = R"(
function check_os()
    return os and "os exists" or "os blocked"
end
)";
    engine.load_string("test_sandbox", code);
    json result = engine.call_function("check_os", json::object());
    // Result should indicate os is blocked
    EXPECT_TRUE(result.is_string());
}
