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

    /**
     * @brief 安全地求值数学/逻辑表达式，返回计算结果。
     *
     * 内部使用 Luau 编译器将 "return <expr>" 编译为字节码后执行，
     * 沙箱环境确保只允许纯计算，无副作用。
     *
     * @param expr 待求值表达式字符串，如 "1 + 2 * 3"
     * @return 计算结果 JSON（number / string / nil）；若编译或执行失败则返回 {"error": ...}
     */
    json evaluate_expression(const std::string& expr);

private:
    void sandbox_environment();
    void push_json_value(const json& value);
    json pop_json_value();

    lua_State* L_ = nullptr;
};

} // namespace agent
