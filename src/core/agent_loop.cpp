#include "agent_loop.hpp"
#include "logger.hpp"
#include "script/script_tool.hpp"
#include "process_engine/response_parser.hpp"

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
    info += "Endpoint: " + api_client_.get_base_url() + constants::API_PATH_MESSAGES + "\n";
    info += "Model: " + api_client_.get_model() + "\n";
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
            // 使用 LuauEngine::evaluate_expression 安全求值
            // 内部将表达式编译为 Luau 字节码并在沙箱中执行，无副作用
            json result = script_engine_->evaluate_expression(expr);
            if (result.contains("error")) {
                return "Calculator error: " + result["error"].get<std::string>();
            }
            // 数值结果直接返回，其他类型序列化为字符串
            if (result.is_number()) {
                return std::to_string(result.get<double>());
            }
            return result.dump();
        }
    );
}

std::string AgentLoop::run(const std::string& user_input) {
    using namespace constants;
    conversation_.add_user_msg(user_input);

    LOG_INFO("AgentLoop: starting with user input, {} tools registered",
             tool_registry_.get_definitions().size());

    for (int iter = 0; iter < max_iterations_; ++iter) {
        LOG_DEBUG("AgentLoop: iteration {}/{}", iter + 1, max_iterations_);

        // 构造 API 请求（内部使用 MessageFormatter 格式化消息）
        auto response = api_client_.messages_create(
            api_client_.get_model(),
            conversation_.get_messages(),
            tool_registry_.get_definitions(),
            max_tokens_);

        if (response.empty()) {
            LOG_ERROR("AgentLoop: empty response from API");
            return "Error: failed to get response from API";
        }

        // ------ 使用 ResponseParser 解析 API 响应 ------

        // 提取 reasoning_content（DeepSeek thinking mode 要求回传）
        std::string reasoning = ResponseParser::extract_reasoning(response);

        // 检查是否应停止
        if (ResponseParser::should_stop(response)) {
            std::string text = ResponseParser::extract_text(response);
            if (!text.empty()) {
                conversation_.add_assistant_msg(text, "", "", {}, reasoning);
            }
            return text;
        }

        // 检查是否包含 tool_use
        if (ResponseParser::has_tool_use(response)) {
            auto tool_use = ResponseParser::extract_tool_use(response);

            std::string tool_id = tool_use.value(JSON_ID, "");
            std::string tool_name = tool_use.value(JSON_NAME, "");
            json tool_input = tool_use.value(JSON_INPUT, json::object());

            LOG_INFO("AgentLoop: tool_use detected: {}", tool_name);

            // 将 assistant 消息（含 tool_use）写入对话历史
            // reasoning_content 必须回传，否则 DeepSeek API 返回 400 错误
            std::string assistant_text = ResponseParser::extract_text(response);
            conversation_.add_assistant_msg(assistant_text, tool_name, tool_id, tool_input, reasoning);

            // 执行工具
            std::string tool_result = tool_registry_.call(tool_name, tool_input);

            // 将 tool_result 写入对话历史
            conversation_.add_tool_result(tool_id, tool_result);

            LOG_DEBUG("AgentLoop: tool result: {}",
                      tool_result.substr(0, 200));
        } else {
            // 纯文本响应 — agent 完成
            std::string text = ResponseParser::extract_text(response);
            if (!text.empty()) {
                conversation_.add_assistant_msg(text, "", "", {}, reasoning);
            }
            return text;
        }
    }

    LOG_WARN("AgentLoop: max iterations ({}) reached", max_iterations_);
    return "Agent stopped: maximum iterations reached.";
}

} // namespace agent
