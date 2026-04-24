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

    if (!tools.empty()) {
        json tools_array = json::array();
        for (const auto& td : tools) {
            json t;
            t[JSON_NAME] = td.name;
            t[JSON_DESCRIPTION] = td.description;
            t[JSON_INPUT_SCHEMA] = td.input_schema;
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
            // tool_result 消息格式
            json block;
            block[JSON_TYPE] = VALUE_TOOL_RESULT;
            block[JSON_TOOL_USE_ID] = m.tool_call_id;
            block[JSON_CONTENT] = m.content;
            msg[JSON_CONTENT] = json::array({block});
        } else if (m.role == VALUE_ASSISTANT && !m.tool_name.empty()) {
            // assistant 消息包含 tool_use 块
            json text_block;
            text_block[JSON_TYPE] = VALUE_TEXT;
            text_block[JSON_TEXT] = m.content;

            json tool_block;
            tool_block[JSON_TYPE] = VALUE_TOOL_USE;
            tool_block[JSON_ID] = m.tool_call_id;
            tool_block[JSON_NAME] = m.tool_name;
            tool_block[JSON_INPUT] = m.tool_input;
            msg[JSON_CONTENT] = json::array({text_block, tool_block});
        } else {
            // 纯文本消息
            msg[JSON_CONTENT] = m.content;
        }

        arr.push_back(msg);
    }
    return arr;
}

} // namespace agent
