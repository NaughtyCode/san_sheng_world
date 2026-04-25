// ============================================================================
// MessageFormatter 实现 — OpenAI API 请求消息格式化器
//
// 将内部数据结构转换为 OpenAI Chat Completions API 兼容的请求体。
// 同时兼容 DeepSeek thinking mode 的 reasoning_content 回传要求。
// ============================================================================

#include "process_engine/message_formatter.hpp"
#include "constants.hpp"
#include "logger.hpp"

namespace agent {

// 模块专属日志标签
#define PE_FMT_TAG "[ProcessEngine::MessageFormatter]"

// ============================================================================
// build_request_body — 构造完整的 API 请求体
// ============================================================================
json MessageFormatter::build_request_body(const std::string& model,
                                           const std::vector<Message>& messages,
                                           const std::vector<ToolDefinition>& tools,
                                           int max_tokens) {
    using namespace constants;
    json body;

    // 基本字段
    body[JSON_MODEL] = model;
    body[JSON_MAX_TOKENS] = max_tokens;
    body[JSON_MESSAGES] = to_api_format(messages);

    // 工具定义（仅在有工具时添加）
    if (!tools.empty()) {
        body[JSON_TOOLS] = format_tools(tools);
        LOG_DEBUG("{} built request: model={}, msg_count={}, tool_count={}",
                  PE_FMT_TAG, model, messages.size(), tools.size());
    } else {
        LOG_DEBUG("{} built request: model={}, msg_count={}, no tools",
                  PE_FMT_TAG, model, messages.size());
    }

    return body;
}

// ============================================================================
// to_api_format — 将内部 Message 列表转换为 API JSON 数组
// ============================================================================
json MessageFormatter::to_api_format(const std::vector<Message>& msgs) {
    using namespace constants;
    json arr = json::array();

    for (const auto& m : msgs) {
        json msg;
        msg[JSON_ROLE] = m.role;

        if (m.role == VALUE_TOOL) {
            // ------ Tool 消息 ------
            // OpenAI 格式：content 为纯字符串，tool_call_id 与 role 平级
            msg[JSON_TOOL_CALL_ID] = m.tool_call_id;
            msg[JSON_CONTENT] = m.content;

        } else if (m.role == VALUE_ASSISTANT && !m.tool_name.empty()) {
            // ------ Assistant 消息（含 tool_use）------
            // OpenAI 格式：content 为纯字符串（可为空），tool_calls 为数组
            msg[JSON_CONTENT] = m.content.empty() ? "" : m.content;

            // DeepSeek thinking mode：reasoning_content 必须回传，否则 400 错误
            if (!m.reasoning_content.empty()) {
                msg[JSON_REASONING_CONTENT] = m.reasoning_content;
            }

            // 构造 tool_calls 数组（OpenAI 格式）
            json tool_call;
            tool_call[JSON_ID] = m.tool_call_id;
            tool_call[JSON_TYPE] = VALUE_FUNCTION;

            // function 子对象：name + arguments（arguments 必须是 JSON 字符串）
            tool_call[JSON_FUNCTION] = json::object();
            tool_call[JSON_FUNCTION][JSON_NAME] = m.tool_name;
            tool_call[JSON_FUNCTION][JSON_ARGUMENTS] = m.tool_input.is_null()
                ? "{}" : m.tool_input.dump();

            msg[JSON_TOOL_CALLS] = json::array({tool_call});

        } else {
            // ------ 纯文本消息（user / assistant 不含 tool_use）------
            // OpenAI 格式：content 为纯字符串
            msg[JSON_CONTENT] = m.content;

            // DeepSeek thinking mode：纯文本 assistant 消息也需回传 reasoning_content
            if (m.role == VALUE_ASSISTANT && !m.reasoning_content.empty()) {
                msg[JSON_REASONING_CONTENT] = m.reasoning_content;
            }
        }

        arr.push_back(msg);
    }

    LOG_DEBUG("{} converted {} messages to API format", PE_FMT_TAG, msgs.size());
    return arr;
}

// ============================================================================
// format_tools — 将 ToolDefinition 列表转换为 API tools JSON 数组
// ============================================================================
json MessageFormatter::format_tools(const std::vector<ToolDefinition>& tools) {
    using namespace constants;
    json tools_array = json::array();

    for (const auto& td : tools) {
        json t;
        // OpenAI 格式：每个工具必须有 type: "function"
        t[JSON_TYPE] = VALUE_FUNCTION;

        // 工具详情包装在 function 子对象中
        json func;
        func[JSON_NAME] = td.name;
        func[JSON_DESCRIPTION] = td.description;
        // OpenAI 使用 parameters（而非 Anthropic 的 input_schema）
        func[JSON_PARAMETERS] = td.input_schema;
        t[JSON_FUNCTION] = func;

        tools_array.push_back(t);
    }

    LOG_DEBUG("{} formatted {} tool definitions", PE_FMT_TAG, tools.size());
    return tools_array;
}

#undef PE_FMT_TAG

} // namespace agent
