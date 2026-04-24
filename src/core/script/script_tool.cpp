#include "script/script_tool.hpp"
#include "logger.hpp"

namespace agent {

ScriptTool::ScriptTool(LuauEngine* engine, std::string function_name)
    : engine_(engine), function_name_(std::move(function_name)) {}

std::string ScriptTool::operator()(const json& args) const {
    if (!engine_) {
        return "Error: script engine not initialized";
    }
    json result = engine_->call_function(function_name_, args);
    if (result.contains("error")) {
        return result["error"].get<std::string>();
    }
    return result.dump();
}

} // namespace agent
