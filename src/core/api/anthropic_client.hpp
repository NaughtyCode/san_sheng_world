#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "api/http_client.hpp"

namespace agent {

using json = nlohmann::json;

struct Message {
    std::string role;       // "user", "assistant", "tool"
    std::string content;    // text content
    std::string tool_call_id;
    std::string tool_name;
    json tool_input;        // parsed tool_use input
};

struct ToolDefinition {
    std::string name;
    std::string description;
    json input_schema;
};

class AnthropicClient {
public:
    AnthropicClient();

    void set_api_key(const std::string& key);
    void set_model(const std::string& model) { model_ = model; }
    void set_base_url(const std::string& url);

    /**
     * Send a Messages API request. Returns the model's response.
     * On tool_use, response content will contain the tool call as a JSON string,
     * and the client must handle resubmission with tool results.
     */
    json messages_create(const std::string& model,
                         const std::vector<Message>& messages,
                         const std::vector<ToolDefinition>& tools = {},
                         int max_tokens = 4096);

private:
    json build_request_body(const std::string& model,
                            const std::vector<Message>& messages,
                            const std::vector<ToolDefinition>& tools,
                            int max_tokens);

    json messages_to_api_format(const std::vector<Message>& msgs);

    HttpClient http_;
    std::string api_key_;
    std::string model_ = "claude-sonnet-4-6";
    std::string base_url_ = "https://api.anthropic.com";
    std::string api_version_ = "2023-06-01";
};

} // namespace agent
