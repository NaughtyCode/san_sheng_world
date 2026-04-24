#include "agent_loop.hpp"
#include "logger.hpp"
#include "script/script_tool.hpp"

namespace agent {

AgentLoop::AgentLoop()
    : script_engine_(std::make_unique<LuauEngine>()) {
    script_engine_->init();
}

void AgentLoop::set_api_key(const std::string& key) {
    api_client_.set_api_key(key);
}

void AgentLoop::set_model(const std::string& model) {
    api_client_.set_model(model);
}

std::string AgentLoop::get_model_info() const {
    std::string info;
    info += "Model: " + api_client_.get_model() + "\n";
    info += "Base URL: " + api_client_.get_base_url() + "\n";
    info += "API Version: " + api_client_.get_api_version() + "\n";
    info += "API Key: " + api_client_.get_api_key_masked() + "\n";
    info += "Max Tokens: " + std::to_string(max_tokens_) + "\n";
    info += "Max Iterations: " + std::to_string(max_iterations_);
    return info;
}

void AgentLoop::register_tool(const std::string& name,
                               const std::string& description,
                               const nlohmann::json& input_schema,
                               ToolHandler handler) {
    tool_registry_.register_tool(name, description, input_schema, std::move(handler));
}

void AgentLoop::load_script_tool(const std::string& path,
                                  const std::string& func_name) {
    script_engine_->load_script(path);

    // 将 Luau 函数包装为工具 handler
    auto handler = [this, func_name](const nlohmann::json& args) -> std::string {
        json result = script_engine_->call_function(func_name, args);
        if (result.contains("error")) {
            return result["error"].get<std::string>();
        }
        if (result.is_string()) {
            return result.get<std::string>();
        }
        return result.dump();
    };

    // 默认输入 schema：接受任意 JSON object
    json schema;
    schema[constants::JSON_TYPE] = constants::JSON_OBJECT;
    schema[constants::JSON_PROPERTIES] = json::object();
    schema[constants::JSON_REQUIRED] = json::array();

    tool_registry_.register_tool(func_name,
                                  "User-defined Luau script: " + func_name,
                                  schema,
                                  handler);
}

void AgentLoop::load_builtin_tools() {
    using namespace constants;

    // ---------- 内置工具：calculator ----------
    json calc_schema;
    calc_schema[JSON_TYPE] = JSON_OBJECT;
    calc_schema[JSON_PROPERTIES] = json::object();
    calc_schema[JSON_PROPERTIES]["expression"] = json::object();
    calc_schema[JSON_PROPERTIES]["expression"][JSON_TYPE] = JSON_STRING;
    calc_schema[JSON_PROPERTIES]["expression"][JSON_DESCRIPTION] =
        "Mathematical expression to evaluate";
    calc_schema[JSON_REQUIRED] = json::array({"expression"});

    tool_registry_.register_tool(
        "calculator",
        "Evaluate a mathematical expression using Luau",
        calc_schema,
        [this](const json& args) -> std::string {
            std::string expr = args.value("expression", "");
            if (expr.empty()) {
                return "Error: expression is required";
            }
            // 使用 Luau 安全地计算数学表达式
            json result = script_engine_->call_function("eval_expr", json::array({expr}));
            if (result.contains("error")) {
                // 通过 load 尝试直接求值作为后备方案
                std::string code = "return " + expr;
                script_engine_->load_string("__calculator__", code);
                return "Calculator result: " + expr;
            }
            return result.dump();
        }
    );
}

std::string AgentLoop::run(const std::string& user_input) {
    using namespace constants;
    conversation_.add_user_msg(user_input);

    Logger::instance().info("AgentLoop: starting with user input, %zu tools registered",
                             tool_registry_.get_definitions().size());

    for (int iter = 0; iter < max_iterations_; ++iter) {
        Logger::instance().debug("AgentLoop: iteration %d/%d", iter + 1, max_iterations_);

        // 构造 API 请求
        auto response = api_client_.messages_create(
            api_client_.get_model(),
            conversation_.get_messages(),
            tool_registry_.get_definitions(),
            max_tokens_);

        if (response.empty()) {
            Logger::instance().error("AgentLoop: empty response from API");
            return "Error: failed to get response from API";
        }

        // 检查是否需要停止
        if (should_stop(response)) {
            std::string text = extract_text(response);
            if (!text.empty()) {
                conversation_.add_assistant_msg(text);
            }
            return text;
        }

        // 检查是否包含 tool_use
        if (has_tool_use(response)) {
            auto tool_use = extract_tool_use(response);

            std::string tool_id = tool_use.value(JSON_ID, "");
            std::string tool_name = tool_use.value(JSON_NAME, "");
            json tool_input = tool_use.value(JSON_INPUT, json::object());

            Logger::instance().info("AgentLoop: tool_use detected: %s", tool_name.c_str());

            // 将 assistant 消息（含 tool_use）写入对话历史
            std::string assistant_text = extract_text(response);
            conversation_.add_assistant_msg(assistant_text, tool_name, tool_id, tool_input);

            // 执行工具
            std::string tool_result = tool_registry_.call(tool_name, tool_input);

            // 将 tool_result 写入对话历史
            conversation_.add_tool_result(tool_id, tool_result);

            Logger::instance().debug("AgentLoop: tool result: %s",
                                      tool_result.substr(0, 200).c_str());
        } else {
            // 纯文本响应 — agent 完成
            std::string text = extract_text(response);
            if (!text.empty()) {
                conversation_.add_assistant_msg(text);
            }
            return text;
        }
    }

    Logger::instance().warning("AgentLoop: max iterations (%d) reached", max_iterations_);
    return "Agent stopped: maximum iterations reached.";
}

bool AgentLoop::has_tool_use(const json& response) const {
    if (!response.contains(constants::JSON_CONTENT)) return false;
    const auto& content = response[constants::JSON_CONTENT];
    if (!content.is_array()) return false;

    for (const auto& block : content) {
        if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
            return true;
        }
    }
    return false;
}

json AgentLoop::extract_tool_use(const json& response) const {
    if (!response.contains(constants::JSON_CONTENT)) return json::object();
    for (const auto& block : response[constants::JSON_CONTENT]) {
        if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
            return block;
        }
    }
    return json::object();
}

std::string AgentLoop::extract_text(const json& response) const {
    if (!response.contains(constants::JSON_CONTENT)) return "";
    const auto& content = response[constants::JSON_CONTENT];
    if (!content.is_array()) return "";

    std::string text;
    for (const auto& block : content) {
        if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TEXT) {
            std::string t = block.value(constants::JSON_TEXT, "");
            if (!t.empty()) {
                if (!text.empty()) text += "\n";
                text += t;
            }
        }
    }
    return text;
}

bool AgentLoop::should_stop(const json& response) const {
    // 检查 stop_reason
    std::string reason = response.value(constants::JSON_STOP_REASON, "");
    if (reason == constants::VALUE_END_TURN) return true;
    if (reason == constants::VALUE_STOP_SEQUENCE) return true;
    if (reason == constants::VALUE_MAX_TOKENS) return true;

    // 如果响应中没有 tool_use（纯文本），也视为完成
    if (!has_tool_use(response)) return true;

    return false;
}

} // namespace agent
