#include "config.hpp"
#include "constants.hpp"
#include <fstream>
#include <cstdlib>

namespace agent {

void Config::load_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    data_.update(nlohmann::json::parse(f, nullptr, false), true);
}

void Config::load_from_env() {
    // 遍历所有需要支持的环境变量，读取后写入 data_

    // ANTHROPIC_API_KEY —— 主要的 API 认证密钥
    const char* api_key = std::getenv(constants::ENV_ANTHROPIC_API_KEY.c_str());
    if (api_key) {
        data_[constants::CONFIG_API_KEY] = std::string(api_key);
    }

    // ANTHROPIC_AUTH_TOKEN —— 备用的认证令牌（优先级低于 API_KEY）
    const char* auth_token = std::getenv(constants::ENV_ANTHROPIC_AUTH_TOKEN.c_str());
    if (auth_token) {
        data_[constants::CONFIG_AUTH_TOKEN] = std::string(auth_token);
    }

    // ANTHROPIC_MODEL —— 主模型名称
    const char* model = std::getenv(constants::ENV_ANTHROPIC_MODEL.c_str());
    if (model) {
        data_[constants::CONFIG_MODEL] = std::string(model);
    }

    // ANTHROPIC_SMALL_FAST_MODEL —— 小型快速模型名称
    const char* small_fast = std::getenv(constants::ENV_ANTHROPIC_SMALL_FAST_MODEL.c_str());
    if (small_fast) {
        data_[constants::CONFIG_SMALL_FAST_MODEL] = std::string(small_fast);
    }

    // ANTHROPIC_CUSTOM_MODEL_OPTION —— 自定义模型选项
    const char* custom_model = std::getenv(constants::ENV_ANTHROPIC_CUSTOM_MODEL_OPTION.c_str());
    if (custom_model) {
        data_[constants::CONFIG_CUSTOM_MODEL_OPTION] = std::string(custom_model);
    }

    // ANTHROPIC_BASE_URL —— API 基础 URL
    const char* base_url = std::getenv(constants::ENV_ANTHROPIC_BASE_URL.c_str());
    if (base_url) {
        data_[constants::CONFIG_BASE_URL] = std::string(base_url);
    }

    // LOG_LEVEL —— 日志级别
    const char* log_level = std::getenv(constants::ENV_LOG_LEVEL.c_str());
    if (log_level) {
        data_[constants::CONFIG_LOG_LEVEL] = std::string(log_level);
    }

    // LOG_FILE —— 日志文件路径
    const char* log_file = std::getenv(constants::ENV_LOG_FILE.c_str());
    if (log_file) {
        data_[constants::CONFIG_LOG_FILE] = std::string(log_file);
    }

    // API_TIMEOUT_MS —— API 超时时间（毫秒）
    const char* timeout_ms = std::getenv(constants::ENV_API_TIMEOUT_MS.c_str());
    if (timeout_ms) {
        data_[constants::CONFIG_TIMEOUT_MS] = std::string(timeout_ms);
    }
}

void Config::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool Config::has(const std::string& key) const {
    return data_.contains(key);
}

} // namespace agent
