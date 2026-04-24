#include "api/anthropic_client.hpp"
#include "logger.hpp"

namespace agent {

AnthropicClient::AnthropicClient() {
    http_.set_base_url(base_url_);
    http_.set_header("Content-Type", "application/json");
}

void AnthropicClient::set_api_key(const std::string& key) {
    api_key_ = key;
    http_.set_header("x-api-key", key);
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

    Logger::instance().debug("AnthropicClient: sending request, model=%s, msg_count=%zu",
                              model.c_str(), messages.size());

    http_.set_header("anthropic-version", api_version_);
    auto resp = http_.post("/v1/messages", body.dump());

    if (resp.status == -1) {
        Logger::instance().error("AnthropicClient: HTTP error: %s", resp.body.c_str());
        return json::object();
    }

    if (resp.status < 200 || resp.status >= 300) {
        Logger::instance().error("AnthropicClient: API error %d: %s", resp.status, resp.body.c_str());
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
    json body;
    body["model"] = model;
    body["max_tokens"] = max_tokens;
    body["messages"] = messages_to_api_format(messages);

    if (!tools.empty()) {
        json tools_array = json::array();
        for (const auto& td : tools) {
            json t;
            t["name"] = td.name;
            t["description"] = td.description;
            t["input_schema"] = td.input_schema;
            tools_array.push_back(t);
        }
        body["tools"] = tools_array;
    }
    return body;
}

json AnthropicClient::messages_to_api_format(const std::vector<Message>& msgs) {
    json arr = json::array();
    for (const auto& m : msgs) {
        json msg;
        msg["role"] = m.role;

        if (m.role == "tool") {
            msg["tool_use_id"] = m.tool_call_id;
            // Send tool result as string content
            json block;
            block["type"] = "tool_result";
            block["tool_use_id"] = m.tool_call_id;
            block["content"] = m.content;
            msg["content"] = json::array({block});
        } else if (m.role == "assistant" && !m.tool_name.empty()) {
            // Assistant message with tool_use blocks
            json text_block;
            text_block["type"] = "text";
            text_block["text"] = m.content;

            json tool_block;
            tool_block["type"] = "tool_use";
            tool_block["id"] = m.tool_call_id;
            tool_block["name"] = m.tool_name;
            tool_block["input"] = m.tool_input;
            msg["content"] = json::array({text_block, tool_block});
        } else {
            msg["content"] = m.content;
        }

        arr.push_back(msg);
    }
    return arr;
}

} // namespace agent
