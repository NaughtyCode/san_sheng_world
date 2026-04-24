#pragma once
#include <string>
#include <memory>
#include "api/anthropic_client.hpp"
#include "conversation.hpp"
#include "tool_registry.hpp"
#include "script/luau_engine.hpp"
#include "constants.hpp"

namespace agent {

class AgentLoop {
public:
    AgentLoop();

    // ---------- API 配置 ----------
    void set_api_key(const std::string& key);
    void set_model(const std::string& model);
    void set_base_url(const std::string& url) { api_client_.set_base_url(url); }
    void set_max_tokens(int max_tokens) { max_tokens_ = max_tokens; }
    void set_max_iterations(int max_iter) { max_iterations_ = max_iter; }

    /**
     * @brief 设置 HTTP 超时时间。
     * @param seconds 超时秒数，透传至底层 AnthropicClient → HttpClient。
     */
    void set_timeout(int seconds) { api_client_.set_timeout(seconds); }

    /**
     * @brief 返回当前模型及连接信息摘要（用于启动时输出）。
     */
    std::string get_model_info() const;

    void register_tool(const std::string& name,
                       const std::string& description,
                       const nlohmann::json& input_schema,
                       ToolHandler handler);

    LuauEngine& script_engine() { return *script_engine_; }
    void load_script_tool(const std::string& path, const std::string& func_name);
    void load_builtin_tools();

    std::string run(const std::string& user_input);

private:
    bool has_tool_use(const nlohmann::json& response) const;
    nlohmann::json extract_tool_use(const nlohmann::json& response) const;
    std::string extract_text(const nlohmann::json& response) const;
    bool should_stop(const nlohmann::json& response) const;

    AnthropicClient api_client_;
    Conversation conversation_;
    ToolRegistry tool_registry_;
    std::unique_ptr<LuauEngine> script_engine_;
    int max_tokens_ = constants::DEFAULT_MAX_TOKENS;
    int max_iterations_ = constants::DEFAULT_MAX_ITERATIONS;
};

} // namespace agent
