#pragma once

// ============================================================================
// MessageFormatter — OpenAI API 请求消息格式化器
//
// 职责：将内部 Message/ToolDefinition 数据结构转换为
// OpenAI Chat Completions API 兼容的 JSON 格式。
//
// 处理以下格式转换：
//   1. 内部 Message 列表 → API messages 数组（含 tool_calls、reasoning_content）
//   2. 内部 ToolDefinition 列表 → API tools 数组
//   3. 完整的 API 请求体组装
//
// 所有方法均为静态方法，无状态，可直接调用。
// ============================================================================

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "api/anthropic_client.hpp"

namespace agent {

using json = nlohmann::json;

class MessageFormatter {
public:
    // ========================================================================
    // build_request_body — 构造完整的 API 请求体 JSON
    //
    // 包含 model、max_tokens、messages、tools（可选）字段。
    // ========================================================================
    static json build_request_body(const std::string& model,
                                   const std::vector<Message>& messages,
                                   const std::vector<ToolDefinition>& tools,
                                   int max_tokens);

    // ========================================================================
    // to_api_format — 将内部 Message 列表转换为 API JSON 数组
    //
    // 转换规则：
    //   - role="tool"  → 含 tool_call_id + content（纯字符串）
    //   - role="assistant" 含 tool_name → 含 tool_calls 数组
    //     （function.arguments 在 OpenAI 中必须是 JSON 字符串）
    //   - 其他 → content 为纯字符串
    //
    // DeepSeek thinking mode：assistant 消息的 reasoning_content 必须回传
    // ========================================================================
    static json to_api_format(const std::vector<Message>& msgs);

    // ========================================================================
    // format_tools — 将 ToolDefinition 列表转换为 API tools JSON 数组
    //
    // 每个工具转换为 { type: "function", function: { name, description, parameters } }
    // ========================================================================
    static json format_tools(const std::vector<ToolDefinition>& tools);
};

} // namespace agent
