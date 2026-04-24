#include "config.hpp"
#include <fstream>
#include <cstdlib>

namespace agent {

void Config::load_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;
    data_.update(nlohmann::json::parse(f, nullptr, false), true);
}

void Config::load_from_env() {
    // Load ANTHROPIC_API_KEY from environment
    const char* key = std::getenv("ANTHROPIC_API_KEY");
    if (key) {
        data_["api_key"] = std::string(key);
    }
    const char* model = std::getenv("ANTHROPIC_MODEL");
    if (model) {
        data_["model"] = std::string(model);
    }
    const char* log_level = std::getenv("LOG_LEVEL");
    if (log_level) {
        data_["log_level"] = std::string(log_level);
    }
    const char* log_file = std::getenv("LOG_FILE");
    if (log_file) {
        data_["log_file"] = std::string(log_file);
    }
    const char* base_url = std::getenv("ANTHROPIC_BASE_URL");
    if (base_url) {
        data_["base_url"] = std::string(base_url);
    }
}

void Config::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

bool Config::has(const std::string& key) const {
    return data_.contains(key);
}

} // namespace agent
