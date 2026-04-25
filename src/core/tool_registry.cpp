#include "tool_registry.hpp"
#include "logger.hpp"

namespace agent {

void ToolRegistry::register_tool(const std::string& name,
                                  const std::string& description,
                                  const json& input_schema,
                                  ToolHandler handler) {
    ToolEntry entry;
    entry.name = name;
    entry.description = description;
    entry.input_schema = input_schema;
    entry.handler = std::move(handler);
    tools_[name] = std::move(entry);
    LOG_INFO("ToolRegistry: registered tool \"{}\"", name);
}

std::string ToolRegistry::call(const std::string& name, const json& args) {
    auto it = tools_.find(name);
    if (it == tools_.end()) {
        LOG_WARN("ToolRegistry: tool \"{}\" not found", name);
        return "Error: tool not found";
    }
    LOG_DEBUG("ToolRegistry: calling tool \"{}\"", name);
    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        LOG_ERROR("ToolRegistry: tool \"{}\" failed: {}", name, e.what());
        return std::string("Error: ") + e.what();
    }
}

bool ToolRegistry::has(const std::string& name) const {
    return tools_.find(name) != tools_.end();
}

std::vector<ToolDefinition> ToolRegistry::get_definitions() const {
    std::vector<ToolDefinition> defs;
    for (const auto& [name, entry] : tools_) {
        ToolDefinition td;
        td.name = entry.name;
        td.description = entry.description;
        td.input_schema = entry.input_schema;
        defs.push_back(std::move(td));
    }
    return defs;
}

} // namespace agent
