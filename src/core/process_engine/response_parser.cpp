// ============================================================================
// ResponseParser 实现 — 大模型 API 响应解析器
//
// 支持 OpenAI Chat Completions 格式和 Anthropic Messages 原始格式。
// 所有方法均为纯函数（无状态、无副作用），线程安全。
// ============================================================================

#include "process_engine/response_parser.hpp"
#include "constants.hpp"
#include "logger.hpp"

namespace agent {

// 模块专属日志标签，便于在日志中区分来源
#define PE_RESP_TAG "[ProcessEngine::ResponseParser]"

// ============================================================================
// get_message — 从 API 响应中提取消息对象
// ============================================================================
json ResponseParser::get_message(const json& response) {
    // OpenAI Chat Completions 格式：
    // 消息嵌套在 response.choices[0].message 中
    if (response.contains(constants::JSON_CHOICES) &&
        response[constants::JSON_CHOICES].is_array() &&
        !response[constants::JSON_CHOICES].empty()) {
        const auto& first_choice = response[constants::JSON_CHOICES][0];
        if (first_choice.contains(constants::JSON_MESSAGE)) {
            return first_choice[constants::JSON_MESSAGE];
        }
    }
    // 兼容 Anthropic Messages 原始格式：
    // 响应根级别直接包含 content 等消息字段
    return response;
}

// ============================================================================
// has_tool_use — 检查响应中是否包含工具调用
// ============================================================================
bool ResponseParser::has_tool_use(const json& response) {
    json msg = get_message(response);

    // ------ OpenAI 格式：检查 message.tool_calls 数组 ------
    if (msg.contains(constants::JSON_TOOL_CALLS) &&
        msg[constants::JSON_TOOL_CALLS].is_array() &&
        !msg[constants::JSON_TOOL_CALLS].empty()) {
        LOG_DEBUG("{} detected tool_use via OpenAI tool_calls", PE_RESP_TAG);
        return true;
    }

    // ------ Anthropic 格式：遍历 content 数组查找 tool_use 块 ------
    if (msg.contains(constants::JSON_CONTENT) && msg[constants::JSON_CONTENT].is_array()) {
        for (const auto& block : msg[constants::JSON_CONTENT]) {
            if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
                LOG_DEBUG("{} detected tool_use via Anthropic content block", PE_RESP_TAG);
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// extract_tool_use — 提取工具调用信息为统一格式 {id, name, input}
// ============================================================================
json ResponseParser::extract_tool_use(const json& response) {
    json msg = get_message(response);

    // ------ OpenAI 格式：从 tool_calls 数组提取第一个工具调用 ------
    if (msg.contains(constants::JSON_TOOL_CALLS) &&
        msg[constants::JSON_TOOL_CALLS].is_array() &&
        !msg[constants::JSON_TOOL_CALLS].empty()) {

        const auto& tc = msg[constants::JSON_TOOL_CALLS][0];
        json result;

        // 提取工具调用 ID
        result[constants::JSON_ID] = tc.value(constants::JSON_ID, "");

        // function.name → name
        if (tc.contains(constants::JSON_FUNCTION)) {
            result[constants::JSON_NAME] =
                tc[constants::JSON_FUNCTION].value(constants::JSON_NAME, "");

            // function.arguments 是 JSON 字符串，需解析为 object
            std::string args_str =
                tc[constants::JSON_FUNCTION].value(constants::JSON_ARGUMENTS, "{}");
            try {
                result[constants::JSON_INPUT] = json::parse(args_str);
            } catch (const json::parse_error& e) {
                LOG_WARN("{} failed to parse tool arguments: {}", PE_RESP_TAG, e.what());
                result[constants::JSON_INPUT] = json::object();
            }
        } else {
            result[constants::JSON_NAME] = "";
            result[constants::JSON_INPUT] = json::object();
        }

        LOG_DEBUG("{} extracted tool_use: name={}, id={}", PE_RESP_TAG,
                  result[constants::JSON_NAME].get<std::string>(),
                  result[constants::JSON_ID].get<std::string>());
        return result;
    }

    // ------ Anthropic 格式：从 content 数组中查找 tool_use 块 ------
    if (msg.contains(constants::JSON_CONTENT)) {
        for (const auto& block : msg[constants::JSON_CONTENT]) {
            if (block.value(constants::JSON_TYPE, "") == constants::VALUE_TOOL_USE) {
                LOG_DEBUG("{} extracted tool_use from Anthropic block", PE_RESP_TAG);
                return block;
            }
        }
    }

    // 未找到工具调用，返回空对象
    LOG_WARN("{} no tool_use found in response", PE_RESP_TAG);
    return json::object();
}

// ============================================================================
// extract_text — 提取响应中的纯文本内容
// ============================================================================
std::string ResponseParser::extract_text(const json& response) {
    json msg = get_message(response);

    // ------ OpenAI 格式：content 是纯字符串 ------
    if (msg.contains(constants::JSON_CONTENT) && msg[constants::JSON_CONTENT].is_string()) {
        std::string text = msg[constants::JSON_CONTENT].get<std::string>();
        LOG_DEBUG("{} extracted text ({} chars) from OpenAI string content",
                  PE_RESP_TAG, text.size());
        return text;
    }

    // ------ Anthropic 格式：content 是数组，收集所有 text 块 ------
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
        LOG_DEBUG("{} extracted text ({} chars) from Anthropic content blocks",
                  PE_RESP_TAG, text.size());
        return text;
    }

    return "";
}

// ============================================================================
// extract_reasoning — 提取 DeepSeek thinking mode 推理内容
// ============================================================================
std::string ResponseParser::extract_reasoning(const json& response) {
    json msg = get_message(response);

    // DeepSeek thinking mode 在 message 中额外返回 reasoning_content 字段
    // 包含模型的推理/思考过程文本
    if (msg.contains(constants::JSON_REASONING_CONTENT) &&
        msg[constants::JSON_REASONING_CONTENT].is_string()) {
        std::string reasoning = msg[constants::JSON_REASONING_CONTENT].get<std::string>();
        LOG_DEBUG("{} extracted reasoning_content ({} chars)", PE_RESP_TAG, reasoning.size());
        return reasoning;
    }
    return "";
}

// ============================================================================
// should_stop — 判断对话是否应结束
// ============================================================================
bool ResponseParser::should_stop(const json& response) {
    // ------ OpenAI 格式：检查 choices[0].finish_reason ------
    if (response.contains(constants::JSON_CHOICES) &&
        response[constants::JSON_CHOICES].is_array() &&
        !response[constants::JSON_CHOICES].empty()) {

        const auto& first_choice = response[constants::JSON_CHOICES][0];
        std::string finish_reason = first_choice.value(constants::JSON_FINISH_REASON, "");

        // "stop" 或 "length"（旧版 API 的 max_tokens）表示正常结束
        if (finish_reason == constants::VALUE_STOP) {
            LOG_DEBUG("{} finish_reason=stop → should stop", PE_RESP_TAG);
            return true;
        }
        if (finish_reason == constants::VALUE_MAX_TOKENS) {
            LOG_DEBUG("{} finish_reason=max_tokens → should stop", PE_RESP_TAG);
            return true;
        }
        // "tool_calls" 表示需要执行工具，不应停止
        if (finish_reason == constants::VALUE_TOOL_CALLS) {
            LOG_DEBUG("{} finish_reason=tool_calls → continue", PE_RESP_TAG);
            return false;
        }

        // 未知 finish_reason：无 tool_use 则停止（保守，避免死循环）
        bool stop = !has_tool_use(response);
        LOG_DEBUG("{} finish_reason={} (unknown), has_tool_use={} → {}",
                  PE_RESP_TAG, finish_reason, !stop, stop ? "stop" : "continue");
        return stop;
    }

    // ------ Anthropic 格式：检查 stop_reason ------
    std::string reason = response.value(constants::JSON_STOP_REASON, "");
    if (reason == constants::VALUE_END_TURN) {
        LOG_DEBUG("{} stop_reason=end_turn → should stop", PE_RESP_TAG);
        return true;
    }
    if (reason == constants::VALUE_STOP_SEQUENCE) {
        LOG_DEBUG("{} stop_reason=stop_sequence → should stop", PE_RESP_TAG);
        return true;
    }
    if (reason == constants::VALUE_MAX_TOKENS) {
        LOG_DEBUG("{} stop_reason=max_tokens → should stop", PE_RESP_TAG);
        return true;
    }

    // 无 tool_use → 纯文本响应，视为完成
    bool stop = !has_tool_use(response);
    LOG_DEBUG("{} no explicit stop_reason, has_tool_use={} → {}",
              PE_RESP_TAG, !stop, stop ? "stop" : "continue");
    return stop;
}

#undef PE_RESP_TAG

} // namespace agent
