#include <iostream>
#include <string>
#include <cstdlib>

#include "logger.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "agent_loop.hpp"
#include "constants.hpp"

using namespace agent;

int main(int argc, char* argv[]) {
    using namespace constants;

    // ==========================================================================
    // 1. 初始化配置 — 从环境变量加载
    // ==========================================================================
    Config config;
    config.load_from_env();

    // ==========================================================================
    // 2. 设置日志
    // ==========================================================================
    Logger& log = Logger::instance();
    std::string log_level = config.get<std::string>(CONFIG_LOG_LEVEL, DEFAULT_LOG_LEVEL);
    std::string log_file = config.get<std::string>(CONFIG_LOG_FILE, "");

    if (log_level == "debug") log.set_level(LogLevel::Debug);
    else if (log_level == "warning") log.set_level(LogLevel::Warning);
    else if (log_level == "error") log.set_level(LogLevel::Error);
    else log.set_level(LogLevel::Info);

    if (!log_file.empty()) {
        log.set_file(log_file);
    }

    log.info("AI Agent starting");

    // ==========================================================================
    // 3. 获取认证凭证 — 优先 ANTHROPIC_API_KEY，其次 ANTHROPIC_AUTH_TOKEN
    // ==========================================================================
    std::string api_key = config.get<std::string>(CONFIG_API_KEY, "");
    if (api_key.empty()) {
        api_key = config.get<std::string>(CONFIG_AUTH_TOKEN, "");
    }
    if (api_key.empty()) {
        std::cerr << "Error: Neither " << ENV_ANTHROPIC_API_KEY
                  << " nor " << ENV_ANTHROPIC_AUTH_TOKEN
                  << " is set. Export one or set in config." << std::endl;
        return 1;
    }

    // ==========================================================================
    // 4. 解析命令行参数
    // ==========================================================================
    bool interactive = true;
    std::string initial_prompt;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-interactive" || arg == "-n") {
            interactive = false;
        } else if (arg == "--model" || arg == "-m") {
            if (i + 1 < argc) {
                config.set(CONFIG_MODEL, argv[++i]);
            }
        } else {
            // 其他参数拼接为初始提示词
            initial_prompt += arg;
            if (i + 1 < argc) initial_prompt += " ";
        }
    }

    // ==========================================================================
    // 5. 创建 Agent 并配置
    // ==========================================================================
    AgentLoop agent;
    agent.set_api_key(api_key);

    // 模型：依次读取命令行覆盖、环境变量、默认值
    std::string model = config.get<std::string>(CONFIG_MODEL, DEFAULT_MODEL);
    agent.set_model(model);

    // 基础 URL
    std::string base_url = config.get<std::string>(CONFIG_BASE_URL, DEFAULT_BASE_URL);
    agent.set_base_url(base_url);

    // 超时时间（毫秒 → 秒）
    int timeout_ms = config.get<int>(CONFIG_TIMEOUT_MS, DEFAULT_TIMEOUT_MS);
    agent.set_timeout(timeout_ms / 1000);

    // Small Fast Model 与 Custom Model Option（暂存于 config，供未来扩展使用）
    std::string small_fast = config.get<std::string>(CONFIG_SMALL_FAST_MODEL, DEFAULT_SMALL_FAST_MODEL);
    std::string custom_opt = config.get<std::string>(CONFIG_CUSTOM_MODEL_OPTION, DEFAULT_CUSTOM_MODEL_OPTION);
    (void)small_fast;
    (void)custom_opt;

    // ==========================================================================
    // 6. 加载内置工具
    // ==========================================================================
    agent.load_builtin_tools();

    log.info("Agent initialized with model: %s", model.c_str());

    // ==========================================================================
    // 7. 输出模型完整信息
    // ==========================================================================
    std::cout << agent.get_model_info() << std::endl;

    // ==========================================================================
    // 8. 一次性提问模式（如果提供了命令行提示词）
    // ==========================================================================
    if (!initial_prompt.empty()) {
        std::string response = agent.run(utils::trim(initial_prompt));
        std::cout << response << std::endl;
    }

    // ==========================================================================
    // 9. 交互式 REPL 模式
    // ==========================================================================
    if (interactive) {
        std::cout << "AI Agent ready. Type 'exit' to quit." << std::endl;
        std::string line;
        while (true) {
            std::cout << "\n> ";
            if (!std::getline(std::cin, line)) break;
            line = utils::trim(line);
            if (line.empty()) continue;
            if (line == "exit" || line == "quit") break;

            std::string response = agent.run(line);
            std::cout << "\n" << response << std::endl;
        }
    }

    log.info("AI Agent shutting down");
    return 0;
}
