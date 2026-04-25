#include "api/anthropic_client.hpp"
#include "logger.hpp"
#include "process_engine/message_formatter.hpp"

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
    // 使用 process_engine::MessageFormatter 构造请求体
    auto body = MessageFormatter::build_request_body(model, messages, tools, max_tokens);

    // 构造完整 URL 用于诊断输出
    std::string full_url = base_url_ + constants::API_PATH_MESSAGES;
    LOG_DEBUG("AnthropicClient: POST {}, model={}, msg_count={}",
              full_url, model, messages.size());

    // OpenAI 兼容端点使用 Bearer 认证方式，DeepSeek 同时支持 x-api-key 和 Authorization
    http_.set_header(constants::API_HEADER_ANTHROPIC_VERSION, api_version_);
    auto resp = http_.post(constants::API_PATH_MESSAGES, body.dump());

    // ------ 连接失败 ------
    if (resp.status == -1) {
        LOG_ERROR("AnthropicClient: HTTP connection failed (url={}): {}",
                  full_url, resp.body);
        return json::object();
    }

    // ------ HTTP 错误码 ------
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
        LOG_ERROR("AnthropicClient: API error {} from {}: {}{}",
                  resp.status, full_url, resp.body, hint);
        return json::object();
    }

    // ------ JSON 解析 ------
    try {
        return json::parse(resp.body);
    } catch (const json::parse_error& e) {
        LOG_ERROR("AnthropicClient: JSON parse error: {}", e.what());
        return json::object();
    }
}

} // namespace agent
