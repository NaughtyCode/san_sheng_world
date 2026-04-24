#pragma once
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "api/anthropic_client.hpp"

namespace agent {

using json = nlohmann::json;
using ToolHandler = std::function<std::string(const json& args)>;

class ToolRegistry {
public:
    ToolRegistry() = default;

    void register_tool(const std::string& name,
                       const std::string& description,
                       const json& input_schema,
                       ToolHandler handler);

    std::string call(const std::string& name, const json& args);
    bool has(const std::string& name) const;
    std::vector<ToolDefinition> get_definitions() const;

private:
    struct ToolEntry {
        std::string name;
        std::string description;
        json input_schema;
        ToolHandler handler;
    };
    std::map<std::string, ToolEntry> tools_;
};

} // namespace agent
