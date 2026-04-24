#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "script/luau_engine.hpp"

namespace agent {

using json = nlohmann::json;

class ScriptTool {
public:
    /// Create a script tool that calls a named Luau function.
    ScriptTool(LuauEngine* engine, std::string function_name);

    std::string operator()(const json& args) const;

private:
    LuauEngine* engine_;
    std::string function_name_;
};

} // namespace agent
