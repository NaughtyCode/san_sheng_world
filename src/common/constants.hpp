#pragma once
#include <string>

/**
 * @file constants.hpp
 * @brief 集中定义工程中所有常量，包括默认值、环境变量名、配置键、API 字符串等。
 *
 * 所有常量在对应的 constants.cpp 中定义，此处仅做 extern 声明。
 * 使用方式： #include "constants.hpp" 后通过 agent::constants::XXX 引用。
 */

namespace agent::constants {

// ============================================================================
// 环境变量名称
// ============================================================================
extern const std::string ENV_ANTHROPIC_API_KEY;             // API 认证密钥
extern const std::string ENV_ANTHROPIC_AUTH_TOKEN;          // API 认证令牌（备用）
extern const std::string ENV_ANTHROPIC_MODEL;               // 主模型名称
extern const std::string ENV_ANTHROPIC_SMALL_FAST_MODEL;    // 小型快速模型名称
extern const std::string ENV_ANTHROPIC_CUSTOM_MODEL_OPTION; // 自定义模型选项
extern const std::string ENV_ANTHROPIC_BASE_URL;            // API 基础 URL
extern const std::string ENV_LOG_LEVEL;                     // 日志级别
extern const std::string ENV_LOG_FILE;                      // 日志文件路径
extern const std::string ENV_API_TIMEOUT_MS;                // API 超时时间（毫秒）

// ============================================================================
// 配置键（Config 中存储使用的 key）
// ============================================================================
extern const std::string CONFIG_API_KEY;             // 认证密钥
extern const std::string CONFIG_AUTH_TOKEN;          // 认证令牌
extern const std::string CONFIG_MODEL;               // 主模型
extern const std::string CONFIG_SMALL_FAST_MODEL;    // 小型快速模型
extern const std::string CONFIG_CUSTOM_MODEL_OPTION; // 自定义模型选项
extern const std::string CONFIG_BASE_URL;            // API 基础 URL
extern const std::string CONFIG_LOG_LEVEL;           // 日志级别
extern const std::string CONFIG_LOG_FILE;            // 日志文件路径
extern const std::string CONFIG_TIMEOUT_MS;          // 超时时间

// ============================================================================
// 默认值
// ============================================================================
extern const std::string DEFAULT_MODEL;               // 默认主模型
extern const std::string DEFAULT_SMALL_FAST_MODEL;    // 默认小型快速模型
extern const std::string DEFAULT_CUSTOM_MODEL_OPTION; // 默认自定义模型选项
extern const std::string DEFAULT_BASE_URL;            // 默认 API 基础 URL
extern const std::string DEFAULT_API_VERSION;         // Anthropic API 版本标头
extern const std::string DEFAULT_LOG_LEVEL;           // 默认日志级别
extern const int         DEFAULT_MAX_TOKENS;          // 默认最大输出 token 数
extern const int         DEFAULT_MAX_ITERATIONS;      // 默认最大工具调用循环次数
extern const int         DEFAULT_TIMEOUT_MS;          // 默认 HTTP 超时（毫秒）

// ============================================================================
// API 通用字符串
// ============================================================================
extern const std::string API_HEADER_CONTENT_TYPE;    // Content-Type 头名称
extern const std::string API_HEADER_X_API_KEY;       // x-api-key 头名称
extern const std::string API_HEADER_ANTHROPIC_VERSION; // anthropic-version 头名称
extern const std::string API_MIME_JSON;              // application/json MIME 类型
extern const std::string API_PATH_MESSAGES;          // /v1/messages 端点路径

// ============================================================================
// JSON 键 — Anthropic Messages API 请求/响应字段
// ============================================================================
extern const std::string JSON_MODEL;          // 请求中的模型字段
extern const std::string JSON_MAX_TOKENS;     // 请求中的 max_tokens 字段
extern const std::string JSON_MESSAGES;       // 请求中的 messages 字段
extern const std::string JSON_TOOLS;          // 请求中的 tools 字段
extern const std::string JSON_ROLE;           // 消息角色
extern const std::string JSON_CONTENT;        // 消息内容
extern const std::string JSON_TYPE;           // 内容块类型
extern const std::string JSON_TEXT;           // 文本字段
extern const std::string JSON_NAME;           // 工具/内容块名称
extern const std::string JSON_DESCRIPTION;    // 工具描述
extern const std::string JSON_INPUT_SCHEMA;   // 工具输入 schema
extern const std::string JSON_INPUT;          // 工具输入
extern const std::string JSON_ID;             // tool_use id
extern const std::string JSON_TOOL_USE_ID;    // tool_result 中的 tool_use_id
extern const std::string JSON_STOP_REASON;    // stop_reason 字段
extern const std::string JSON_PROPERTIES;     // JSON Schema properties
extern const std::string JSON_REQUIRED;       // JSON Schema required
extern const std::string JSON_OBJECT;         // JSON Schema type "object"
extern const std::string JSON_STRING;         // JSON Schema type "string"
extern const std::string JSON_FUNCTION;       // OpenAI tool/function wrapper key
extern const std::string JSON_PARAMETERS;     // OpenAI tool parameters key（对应 Anthropic 的 input_schema）
extern const std::string JSON_TOOL_CALLS;     // OpenAI assistant 消息中的 tool_calls 字段
extern const std::string JSON_ARGUMENTS;      // OpenAI tool_calls 中的 arguments 字段
extern const std::string JSON_CHOICES;        // OpenAI 响应中的 choices 数组
extern const std::string JSON_MESSAGE;        // OpenAI 响应 choice 中的 message 对象
extern const std::string JSON_FINISH_REASON;  // OpenAI 响应中的 finish_reason 字段
extern const std::string JSON_DELTA;          // OpenAI streaming chunk 中的 delta
extern const std::string JSON_INDEX;          // OpenAI choice 的索引字段

// ============================================================================
// JSON 值 — 固定的字符串取值
// ============================================================================
extern const std::string VALUE_TOOL_USE;       // content block type: tool_use
extern const std::string VALUE_TOOL_RESULT;    // content block type: tool_result
extern const std::string VALUE_TEXT;           // content block type: text
extern const std::string VALUE_USER;           // role: user
extern const std::string VALUE_ASSISTANT;      // role: assistant
extern const std::string VALUE_TOOL;           // role: tool
extern const std::string VALUE_END_TURN;       // stop_reason: end_turn
extern const std::string VALUE_STOP_SEQUENCE;  // stop_reason: stop_sequence
extern const std::string VALUE_MAX_TOKENS;     // stop_reason: max_tokens
extern const std::string VALUE_FUNCTION;       // OpenAI tool type: "function"
extern const std::string VALUE_TOOL_CALLS;     // OpenAI finish_reason: "tool_calls"
extern const std::string VALUE_STOP;           // OpenAI finish_reason: "stop"

} // namespace agent::constants
