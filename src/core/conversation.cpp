#include "conversation.hpp"

namespace agent {

void Conversation::add_user_msg(const std::string& content) {
    Message m;
    m.role = "user";
    m.content = content;
    messages_.push_back(std::move(m));
}

void Conversation::add_assistant_msg(const std::string& content,
                                      const std::string& tool_name,
                                      const std::string& tool_id,
                                      const json& tool_input,
                                      const std::string& reasoning_content) {
    Message m;
    m.role = "assistant";
    m.content = content;
    m.tool_name = tool_name;
    m.tool_call_id = tool_id;
    m.tool_input = tool_input;
    m.reasoning_content = reasoning_content;
    messages_.push_back(std::move(m));
}

void Conversation::add_tool_result(const std::string& tool_call_id,
                                    const std::string& result) {
    Message m;
    m.role = "tool";
    m.tool_call_id = tool_call_id;
    m.content = result;
    messages_.push_back(std::move(m));
}

} // namespace agent
