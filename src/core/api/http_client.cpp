#include "api/http_client.hpp"
#include "logger.hpp"
#include <httplib.h>
#include <stdexcept>

namespace agent {

// 从 base_url 提取 scheme + host + port 字符串用于 httplib 连接
static std::string build_client_host(const std::string& base_url) {
    std::string host = base_url;
    if (!host.empty() && host.back() == '/') host.pop_back();
    return host;
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body) {
    HttpResponse resp;
    try {
        httplib::Client cli(build_client_host(base_url_));
        cli.set_connection_timeout(timeout_, 0);
        cli.set_read_timeout(timeout_, 0);
        cli.set_write_timeout(timeout_, 0);

        httplib::Headers httplib_headers;
        for (const auto& [k, v] : headers_) {
            httplib_headers.emplace(k, v);
        }

        auto res = cli.Post(path, httplib_headers, body, constants::API_MIME_JSON);

        if (res) {
            resp.status = res->status;
            resp.body = res->body;
        } else {
            resp.status = -1;
            resp.body = "HTTP request failed";
            LOG_ERROR("HTTP POST failed: {}", httplib::to_string(res.error()));
        }
    } catch (const std::exception& e) {
        resp.status = -1;
        resp.body = std::string("HTTP error: ") + e.what();
        LOG_ERROR("HTTP POST exception: {}", e.what());
    }
    return resp;
}

HttpResponse HttpClient::get(const std::string& path) {
    HttpResponse resp;
    try {
        httplib::Client cli(build_client_host(base_url_));
        cli.set_connection_timeout(timeout_, 0);
        cli.set_read_timeout(timeout_, 0);

        httplib::Headers httplib_headers;
        for (const auto& [k, v] : headers_) {
            httplib_headers.emplace(k, v);
        }

        auto res = cli.Get(path, httplib_headers);

        if (res) {
            resp.status = res->status;
            resp.body = res->body;
        } else {
            resp.status = -1;
            resp.body = "HTTP request failed";
        }
    } catch (const std::exception& e) {
        resp.status = -1;
        resp.body = std::string("HTTP error: ") + e.what();
        LOG_ERROR("HTTP GET exception: {}", e.what());
    }
    return resp;
}

} // namespace agent
