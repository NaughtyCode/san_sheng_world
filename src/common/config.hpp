#pragma once
#include <string>
#include <unordered_map>
#include <sstream>
#include <nlohmann/json.hpp>

namespace agent {

class Config {
public:
    Config() = default;

    void load_from_file(const std::string& path);
    void load_from_env();

    template<typename T>
    T get(const std::string& key, const T& default_val = T{}) const;

    void set(const std::string& key, const std::string& value);
    bool has(const std::string& key) const;

private:
    nlohmann::json data_;
};

template<typename T>
T Config::get(const std::string& key, const T& default_val) const {
    if (!data_.contains(key)) return default_val;
    try {
        const auto& val = data_.at(key);
        // nlohmann::json does not auto-convert strings to numbers,
        // so parse string values manually for numeric types
        if (val.is_string()) {
            if constexpr (std::is_arithmetic_v<T>) {
                T result{};
                std::istringstream iss(val.get<std::string>());
                if (iss >> result) return result;
            }
            return default_val;
        }
        return val.get<T>();
    } catch (...) {
        return default_val;
    }
}

template<>
inline std::string Config::get<std::string>(const std::string& key, const std::string& default_val) const {
    if (!data_.contains(key)) return default_val;
    try {
        if (data_.at(key).is_string()) return data_.at(key).get<std::string>();
        return data_.at(key).dump();
    } catch (...) {
        return default_val;
    }
}

} // namespace agent
