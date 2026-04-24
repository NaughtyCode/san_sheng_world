#include "constants.hpp"

/**
 * @file constants.cpp
 * @brief 工程中所有常量的唯一定义点。
 *
 * 需要新增或修改任何常量时，请在此文件中操作，
 * 并在 constants.hpp 中添加对应的 extern 声明。
 */

namespace agent::constants {

// ============================================================================
// 环境变量名称
// ============================================================================
const std::string ENV_ANTHROPIC_API_KEY("ANTHROPIC_API_KEY");
const std::string ENV_ANTHROPIC_AUTH_TOKEN("ANTHROPIC_AUTH_TOKEN");
const std::string ENV_ANTHROPIC_MODEL("ANTHROPIC_MODEL");
const std::string ENV_ANTHROPIC_SMALL_FAST_MODEL("ANTHROPIC_SMALL_FAST_MODEL");
const std::string ENV_ANTHROPIC_CUSTOM_MODEL_OPTION("ANTHROPIC_CUSTOM_MODEL_OPTION");
const std::string ENV_ANTHROPIC_BASE_URL("ANTHROPIC_BASE_URL");
const std::string ENV_LOG_LEVEL("LOG_LEVEL");
const std::string ENV_LOG_FILE("LOG_FILE");
const std::string ENV_API_TIMEOUT_MS("API_TIMEOUT_MS");

// ============================================================================
// 配置键
// ============================================================================
const std::string CONFIG_API_KEY("api_key");
const std::string CONFIG_AUTH_TOKEN("auth_token");
const std::string CONFIG_MODEL("model");
const std::string CONFIG_SMALL_FAST_MODEL("small_fast_model");
const std::string CONFIG_CUSTOM_MODEL_OPTION("custom_model_option");
const std::string CONFIG_BASE_URL("base_url");
const std::string CONFIG_LOG_LEVEL("log_level");
const std::string CONFIG_LOG_FILE("log_file");
const std::string CONFIG_TIMEOUT_MS("timeout_ms");

// ============================================================================
// 默认值
// ============================================================================
const std::string DEFAULT_MODEL("deepseek-v4-pro");
const std::string DEFAULT_SMALL_FAST_MODEL("deepseek-v4-pro");
const std::string DEFAULT_CUSTOM_MODEL_OPTION("deepseek-v4-pro");
const std::string DEFAULT_BASE_URL("https://api.deepseek.com/anthropic");
const std::string DEFAULT_API_VERSION("2023-06-01");
const std::string DEFAULT_LOG_LEVEL("info");
const int         DEFAULT_MAX_TOKENS = 4096;
const int         DEFAULT_MAX_ITERATIONS = 10;
const int         DEFAULT_TIMEOUT_MS = 600000;

// ============================================================================
// API 通用字符串
// ============================================================================
const std::string API_HEADER_CONTENT_TYPE("Content-Type");
const std::string API_HEADER_X_API_KEY("x-api-key");
const std::string API_HEADER_ANTHROPIC_VERSION("anthropic-version");
const std::string API_MIME_JSON("application/json");
const std::string API_PATH_MESSAGES("/chat/completions");

// ============================================================================
// JSON 键
// ============================================================================
const std::string JSON_MODEL("model");
const std::string JSON_MAX_TOKENS("max_tokens");
const std::string JSON_MESSAGES("messages");
const std::string JSON_TOOLS("tools");
const std::string JSON_ROLE("role");
const std::string JSON_CONTENT("content");
const std::string JSON_TYPE("type");
const std::string JSON_TEXT("text");
const std::string JSON_NAME("name");
const std::string JSON_DESCRIPTION("description");
const std::string JSON_INPUT_SCHEMA("input_schema");
const std::string JSON_INPUT("input");
const std::string JSON_ID("id");
const std::string JSON_TOOL_USE_ID("tool_use_id");
const std::string JSON_STOP_REASON("stop_reason");
const std::string JSON_PROPERTIES("properties");
const std::string JSON_REQUIRED("required");
const std::string JSON_OBJECT("object");
const std::string JSON_STRING("string");

// ============================================================================
// JSON 值
// ============================================================================
const std::string VALUE_TOOL_USE("tool_use");
const std::string VALUE_TOOL_RESULT("tool_result");
const std::string VALUE_TEXT("text");
const std::string VALUE_USER("user");
const std::string VALUE_ASSISTANT("assistant");
const std::string VALUE_TOOL("tool");
const std::string VALUE_END_TURN("end_turn");
const std::string VALUE_STOP_SEQUENCE("stop_sequence");
const std::string VALUE_MAX_TOKENS("max_tokens");

} // namespace agent::constants
