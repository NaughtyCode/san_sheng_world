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
    Logger::instance().info("ToolRegistry: registered tool \"%s\"", name.c_str());
}

std::string ToolRegistry::call(const std::string& name, const json& args) {
    auto it = tools_.find(name);
    if (it == tools_.end()) {
        Logger::instance().warning("ToolRegistry: tool \"%s\" not found", name.c_str());
        return "Error: tool not found";
    }
    Logger::instance().debug("ToolRegistry: calling tool \"%s\"", name.c_str());
    try {
        return it->second.handler(args);
    } catch (const std::exception& e) {
        Logger::instance().error("ToolRegistry: tool \"%s\" failed: %s", name.c_str(), e.what());
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
