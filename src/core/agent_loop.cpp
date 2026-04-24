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

        // 提取 reasoning_content（DeepSeek thinking mode 要求回传）
        std::string reasoning = extract_reasoning(response);

        // 检查是否需要停止
        if (should_stop(response)) {
            std::string text = extract_text(response);
            if (!text.empty()) {
                conversation_.add_assistant_msg(text, "", "", {}, reasoning);
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
            // reasoning_content 必须回传，否则 DeepSeek API 返回 400 错误
            std::string assistant_text = extract_text(response);
            conversation_.add_assistant_msg(assistant_text, tool_name, tool_id, tool_input, reasoning);

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
                conversation_.add_assistant_msg(text, "", "", {}, reasoning);
            }
            return text;
        }
    }

    Logger::instance().warning("AgentLoop: max iterations (%d) reached", max_iterations_);
    return "Agent stopped: maximum iterations reached.";
}

// 解析 OpenAI 响应：从 choices[0].message 中提取消息对象
static json get_message_from_response(const json& response) {
    if (response.contains(constants::JSON_CHOICES) &&
        response[constants::JSON_CHOICES].is_array() &&
        !response[constants::JSON_CHOICES].empty()) {
        const auto& first_choice = response[constants::JSON_CHOICES][0];
        if (first_choice.contains(constants::JSON_MESSAGE)) {
            return first_choice[constants::JSON_MESSAGE];
        }
    }
    // 兼容 Anthropic 原始格式：直接在根级别包含 content
    return response;
}

bool AgentLoop::has_tool_use(const json& response) const {
    json msg = get_message_from_response(response);

    // OpenAI 格式：检查 message.tool_calls 数组
    if (msg.contains(constants::JSON_TOOL_CALLS) &&
        msg[constants::JSON_TOOL_CALLS].is_array() &&
        !msg[constants::JSON_TOOL_CALLS].empty()) {
        return true;
    }

    // 兼容 Anthropic 原始格式：检查 content 数组中的 tool_use 块
    if (msg.contains(constants::JSON_CONTENT) && msg[constants::JSON_CONTENT].is_array()) {
        for (const auto& block : msg[constants::JSON_CONTENT]) {
            if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
                return true;
            }
        }
    }
    return false;
}

json AgentLoop::extract_tool_use(const json& response) const {
    json msg = get_message_from_response(response);

    // OpenAI 格式：从 tool_calls 数组中提取第一个工具调用
    // 并转换成内部统一格式 {id, name, input}
    if (msg.contains(constants::JSON_TOOL_CALLS) &&
        msg[constants::JSON_TOOL_CALLS].is_array() &&
        !msg[constants::JSON_TOOL_CALLS].empty()) {
        const auto& tc = msg[constants::JSON_TOOL_CALLS][0];
        json result;
        result[constants::JSON_ID] = tc.value(constants::JSON_ID, "");

        // function.name 映射为 name
        if (tc.contains(constants::JSON_FUNCTION)) {
            result[constants::JSON_NAME] =
                tc[constants::JSON_FUNCTION].value(constants::JSON_NAME, "");

            // function.arguments 是 JSON 字符串，需要解析为 input
            std::string args_str =
                tc[constants::JSON_FUNCTION].value(constants::JSON_ARGUMENTS, "{}");
            try {
                result[constants::JSON_INPUT] = json::parse(args_str);
            } catch (const json::parse_error&) {
                result[constants::JSON_INPUT] = json::object();
            }
        } else {
            result[constants::JSON_NAME] = "";
            result[constants::JSON_INPUT] = json::object();
        }
        return result;
    }

    // 兼容 Anthropic 原始格式
    if (msg.contains(constants::JSON_CONTENT)) {
        for (const auto& block : msg[constants::JSON_CONTENT]) {
            if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
                return block;
            }
        }
    }
    return json::object();
}

std::string AgentLoop::extract_text(const json& response) const {
    json msg = get_message_from_response(response);

    // OpenAI 格式：content 是纯字符串
    if (msg.contains(constants::JSON_CONTENT) && msg[constants::JSON_CONTENT].is_string()) {
        return msg[constants::JSON_CONTENT].get<std::string>();
    }

    // 兼容 Anthropic 原始格式：content 是数组
    if (msg.contains(constants::JSON_CONTENT) && msg[constants::JSON_CONTENT].is_array()) {
        std::string text;
        for (const auto& block : msg[constants::JSON_CONTENT]) {
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
    return "";
}

std::string AgentLoop::extract_reasoning(const json& response) const {
    json msg = get_message_from_response(response);

    // DeepSeek thinking mode 在 message 中返回 reasoning_content 字段
    if (msg.contains(constants::JSON_REASONING_CONTENT) &&
        msg[constants::JSON_REASONING_CONTENT].is_string()) {
        return msg[constants::JSON_REASONING_CONTENT].get<std::string>();
    }
    return "";
}

bool AgentLoop::should_stop(const json& response) const {
    // OpenAI 格式：检查 choices[0].finish_reason
    if (response.contains(constants::JSON_CHOICES) &&
        response[constants::JSON_CHOICES].is_array() &&
        !response[constants::JSON_CHOICES].empty()) {
        const auto& first_choice = response[constants::JSON_CHOICES][0];
        std::string finish_reason = first_choice.value(constants::JSON_FINISH_REASON, "");

        // "stop" 或 "length"（max_tokens）表示对话结束
        if (finish_reason == constants::VALUE_STOP) return true;
        if (finish_reason == constants::VALUE_MAX_TOKENS) return true;
        // "tool_calls" 表示需要执行工具，不应停止
        if (finish_reason == constants::VALUE_TOOL_CALLS) return false;

        // 如果 finish_reason 未知且没有 tool_use，则停止
        if (!has_tool_use(response)) return true;
        return false;
    }

    // 兼容 Anthropic 原始格式：检查 stop_reason
    std::string reason = response.value(constants::JSON_STOP_REASON, "");
    if (reason == constants::VALUE_END_TURN) return true;
    if (reason == constants::VALUE_STOP_SEQUENCE) return true;
    if (reason == constants::VALUE_MAX_TOKENS) return true;

    // 如果响应中没有 tool_use（纯文本），也视为完成
    if (!has_tool_use(response)) return true;

    return false;
}

} // namespace agent
