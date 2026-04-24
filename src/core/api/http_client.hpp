#pragma once
#include <string>
#include <map>
#include <utility>
#include "constants.hpp"

namespace agent {

struct HttpResponse {
    int status = 0;
    std::string body;
    std::map<std::string, std::string> headers;
};

class HttpClient {
public:
    HttpClient() = default;
    explicit HttpClient(std::string base_url) : base_url_(std::move(base_url)) {}

    void set_base_url(const std::string& url) { base_url_ = url; }
    void set_header(const std::string& key, const std::string& value) { headers_[key] = value; }
    void set_timeout(int seconds) { timeout_ = seconds; }

    HttpResponse post(const std::string& path, const std::string& body);
    HttpResponse get(const std::string& path);

private:
    std::string base_url_;
    std::map<std::string, std::string> headers_;
    int timeout_ = constants::DEFAULT_TIMEOUT_MS / 1000;  // 默认超时（秒），从毫秒常量换算
};

} // namespace agent
