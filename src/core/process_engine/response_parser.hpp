#pragma once

// ============================================================================
// ResponseParser — 大模型 API 响应解析器
//
// 职责：从 LLM API 返回的 JSON 响应中提取结构化信息。
// 支持 OpenAI Chat Completions 格式，同时兼容 Anthropic Messages 原始格式。
//
// 所有方法均为静态方法，无状态，可直接调用。
// ============================================================================

#include <string>
#include <nlohmann/json.hpp>

namespace agent {

using json = nlohmann::json;

class ResponseParser {
public:
    // ========================================================================
    // get_message — 从 API 响应中提取消息对象
    //
    // OpenAI 格式：response.choices[0].message
    // Anthropic 格式：response 本身就是消息（包含 content/tool_use 等）
    // ========================================================================
    static json get_message(const json& response);

    // ========================================================================
    // has_tool_use — 检查响应中是否包含工具调用
    //
    // OpenAI 格式：检查 message.tool_calls 数组
    // Anthropic 格式：遍历 content 数组查找 type="tool_use" 的块
    // ========================================================================
    static bool has_tool_use(const json& response);

    // ========================================================================
    // extract_tool_use — 提取工具调用信息为统一格式
    //
    // 无论输入是 OpenAI 还是 Anthropic 格式，输出统一为：
    //   { "id": "...", "name": "...", "input": {...} }
    //
    // OpenAI 格式中的 function.arguments（JSON 字符串）会被解析为 object
    // ========================================================================
    static json extract_tool_use(const json& response);

    // ========================================================================
    // extract_text — 提取响应中的纯文本内容
    //
    // OpenAI 格式：content 是纯字符串
    // Anthropic 格式：遍历 content 数组，收集所有 type="text" 的块
    // ========================================================================
    static std::string extract_text(const json& response);

    // ========================================================================
    // extract_reasoning — 提取 DeepSeek thinking mode 推理内容
    //
    // DeepSeek v4-pro 等模型在思考模式下的 reasoning_content 字段
    // 必须在后续请求中回传，否则 API 返回 400 错误
    // ========================================================================
    static std::string extract_reasoning(const json& response);

    // ========================================================================
    // should_stop — 判断对话是否应结束
    //
    // OpenAI 格式：检查 choices[0].finish_reason
    //   - "stop" / "max_tokens" → 停止
    //   - "tool_calls" → 继续（需执行工具）
    //   - 未知且无 tool_use → 停止（保守处理）
    //
    // Anthropic 格式：检查 stop_reason
    //   - "end_turn" / "stop_sequence" / "max_tokens" → 停止
    // ========================================================================
    static bool should_stop(const json& response);
};

} // namespace agent
