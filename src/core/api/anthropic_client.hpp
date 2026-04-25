#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "api/http_client.hpp"
#include "constants.hpp"

namespace agent {

using json = nlohmann::json;

/// 内部消息结构：对话历史中的一条消息
struct Message {
    std::string role;       // 消息角色：user / assistant / tool
    std::string content;    // 文本内容
    std::string tool_call_id; // tool_use 对应的 id
    std::string tool_name;  // 被调用的工具名称
    json tool_input;        // 工具调用参数（已解析的 JSON）
    std::string reasoning_content; // DeepSeek thinking mode 推理内容（需在后续请求中回传）
};

/// 工具定义结构：向 API 声明可用工具
struct ToolDefinition {
    std::string name;             // 工具名称
    std::string description;      // 工具描述
    json input_schema;            // JSON Schema 格式的输入参数定义
};

class AnthropicClient {
public:
    AnthropicClient();

    // ---------- 配置 ----------
    void set_api_key(const std::string& key);
    void set_model(const std::string& model) { model_ = model; }
    void set_base_url(const std::string& url);

    /**
     * @brief 设置 HTTP 请求超时时间。
     * @param seconds 超时秒数，传递给底层 HttpClient。
     */
    void set_timeout(int seconds) { http_.set_timeout(seconds); }

    // ---------- 访问器 ----------
    std::string get_model() const { return model_; }
    std::string get_base_url() const { return base_url_; }
    std::string get_api_version() const { return api_version_; }
    std::string get_api_key_masked() const {
        if (api_key_.length() <= 8) return std::string(api_key_.length(), '*');
        return api_key_.substr(0, 7) + "****" + api_key_.substr(api_key_.length() - 4);
    }

    /**
     * @brief 发送 Messages API 请求。
     *
     * 内部使用 process_engine::MessageFormatter 构造请求体，
     * 通过 HttpClient 发送 POST 请求，并处理 HTTP 错误码和 JSON 解析。
     *
     * @param model       模型名称
     * @param messages    对话历史
     * @param tools       可用工具列表（可选）
     * @param max_tokens  最大输出 token 数
     * @return API 响应 JSON；失败时返回空 object
     */
    json messages_create(const std::string& model,
                         const std::vector<Message>& messages,
                         const std::vector<ToolDefinition>& tools = {},
                         int max_tokens = constants::DEFAULT_MAX_TOKENS);

private:
    HttpClient http_;
    std::string api_key_;
    std::string model_ = constants::DEFAULT_MODEL;
    std::string base_url_ = constants::DEFAULT_BASE_URL;
    std::string api_version_ = constants::DEFAULT_API_VERSION;
};

} // namespace agent
