#pragma once
#include <string>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

struct lua_State;

namespace agent {

using json = nlohmann::json;

class LuauEngine {
public:
    LuauEngine();
    ~LuauEngine();

    LuauEngine(const LuauEngine&) = delete;
    LuauEngine& operator=(const LuauEngine&) = delete;

    void init();
    void load_script(const std::string& path);
    void load_string(const std::string& name, const std::string& source);
    json call_function(const std::string& name, const json& args);
    void register_cpp_function(const std::string& name,
                               std::function<json(const json&)> handler);

private:
    void sandbox_environment();
    void push_json_value(const json& value);
    json pop_json_value();

    lua_State* L_ = nullptr;
};

} // namespace agent
