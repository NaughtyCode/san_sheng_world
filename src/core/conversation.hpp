#pragma once
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "api/anthropic_client.hpp"

namespace agent {

using json = nlohmann::json;

class Conversation {
public:
    Conversation() = default;

    void add_user_msg(const std::string& content);
    void add_assistant_msg(const std::string& content,
                           const std::string& tool_name = "",
                           const std::string& tool_id = "",
                           const json& tool_input = {},
                           const std::string& reasoning_content = "");
    void add_tool_result(const std::string& tool_call_id,
                         const std::string& result);

    const std::vector<Message>& get_messages() const { return messages_; }
    void clear() { messages_.clear(); }
    bool empty() const { return messages_.empty(); }

private:
    std::vector<Message> messages_;
};

} // namespace agent
