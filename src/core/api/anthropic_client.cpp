#include "api/anthropic_client.hpp"
#include "logger.hpp"

namespace agent {

AnthropicClient::AnthropicClient() {
    http_.set_base_url(base_url_);
    http_.set_header(constants::API_HEADER_CONTENT_TYPE, constants::API_MIME_JSON);
}

void AnthropicClient::set_api_key(const std::string& key) {
    api_key_ = key;
    http_.set_header(constants::API_HEADER_X_API_KEY, key);
}

void AnthropicClient::set_base_url(const std::string& url) {
    base_url_ = url;
    http_.set_base_url(url);
}

json AnthropicClient::messages_create(const std::string& model,
                                       const std::vector<Message>& messages,
                                       const std::vector<ToolDefinition>& tools,
                                       int max_tokens) {
    auto body = build_request_body(model, messages, tools, max_tokens);

    // 构造完整 URL 用于诊断输出
    std::string full_url = base_url_ + constants::API_PATH_MESSAGES;
    Logger::instance().debug("AnthropicClient: POST %s, model=%s, msg_count=%zu",
                              full_url.c_str(), model.c_str(), messages.size());

    // OpenAI 兼容端点使用 Bearer 认证方式，但 DeepSeek 同时支持 x-api-key 和 Authorization 头
    http_.set_header(constants::API_HEADER_ANTHROPIC_VERSION, api_version_);
    auto resp = http_.post(constants::API_PATH_MESSAGES, body.dump());

    if (resp.status == -1) {
        Logger::instance().error("AnthropicClient: HTTP connection failed (url=%s): %s",
                                  full_url.c_str(), resp.body.c_str());
        return json::object();
    }

    if (resp.status < 200 || resp.status >= 300) {
        // 针对常见 HTTP 错误码提供排查建议
        std::string hint;
        switch (resp.status) {
            case 400: hint = " [Bad Request — check request body format]"; break;
            case 401: hint = " [Unauthorized — check API key]"; break;
            case 403: hint = " [Forbidden — check API key permissions]"; break;
            case 404: hint = " [Not Found — check API endpoint: "
                       + base_url_ + constants::API_PATH_MESSAGES + "]"; break;
            case 429: hint = " [Rate Limited — retry later]"; break;
            case 500: hint = " [Internal Server Error — API-side issue]"; break;
            default: break;
        }
        Logger::instance().error("AnthropicClient: API error %d from %s: %s%s",
                                  resp.status, full_url.c_str(), resp.body.c_str(), hint);
        return json::object();
    }

    try {
        return json::parse(resp.body);
    } catch (const json::parse_error& e) {
        Logger::instance().error("AnthropicClient: JSON parse error: %s", e.what());
        return json::object();
    }
}

json AnthropicClient::build_request_body(const std::string& model,
                                          const std::vector<Message>& messages,
                                          const std::vector<ToolDefinition>& tools,
                                          int max_tokens) {
    using namespace constants;
    json body;
    body[JSON_MODEL] = model;
    body[JSON_MAX_TOKENS] = max_tokens;
    body[JSON_MESSAGES] = messages_to_api_format(messages);

    // 使用 OpenAI Chat Completions 格式构造 tools 数组
    // 每个工具对象包含 type: "function" 和一个 function 子对象
    if (!tools.empty()) {
        json tools_array = json::array();
        for (const auto& td : tools) {
            json t;
            // OpenAI 格式要求每个 tool 必须有 type 字段
            t[JSON_TYPE] = VALUE_FUNCTION;

            // 工具详情包装在 function 对象中
            json func;
            func[JSON_NAME] = td.name;
            func[JSON_DESCRIPTION] = td.description;
            // OpenAI 使用 parameters 而非 Anthropic 的 input_schema
            func[JSON_PARAMETERS] = td.input_schema;
            t[JSON_FUNCTION] = func;

            tools_array.push_back(t);
        }
        body[JSON_TOOLS] = tools_array;
    }
    return body;
}

json AnthropicClient::messages_to_api_format(const std::vector<Message>& msgs) {
    using namespace constants;
    json arr = json::array();
    for (const auto& m : msgs) {
        json msg;
        msg[JSON_ROLE] = m.role;

        if (m.role == VALUE_TOOL) {
            // OpenAI 格式 — tool 角色消息
            // tool 消息的 content 为纯字符串，tool_call_id 与 role 平级
            msg[JSON_TOOL_USE_ID] = m.tool_call_id;
            msg[JSON_CONTENT] = m.content;
        } else if (m.role == VALUE_ASSISTANT && !m.tool_name.empty()) {
            // OpenAI 格式 — assistant 消息包含 tool_calls 数组
            // content 为纯字符串（可为空），tool_calls 为数组
            msg[JSON_CONTENT] = m.content.empty() ? "" : m.content;

            json tool_call;
            tool_call[JSON_ID] = m.tool_call_id;
            tool_call[JSON_TYPE] = VALUE_FUNCTION;
            // arguments 在 OpenAI 中必须是 JSON 字符串
            tool_call[JSON_FUNCTION] = json::object();
            tool_call[JSON_FUNCTION][JSON_NAME] = m.tool_name;
            tool_call[JSON_FUNCTION][JSON_ARGUMENTS] = m.tool_input.is_null()
                ? "{}" : m.tool_input.dump();

            msg[JSON_TOOL_CALLS] = json::array({tool_call});
        } else {
            // 纯文本消息 — user / assistant（不含 tool_use）均为此分支
            // OpenAI 格式中 content 为纯字符串
            msg[JSON_CONTENT] = m.content;
        }

        arr.push_back(msg);
    }
    return arr;
}

} // namespace agent
