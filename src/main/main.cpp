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
    // 1. 初始化新日志系统（基于 spdlog）
    //    读取环境变量配置日志级别和输出文件
    // ==========================================================================
    LogManager::instance().init("log_config.json", "APP_LOG_FILE");

    // ==========================================================================
    // 2. 初始化配置 — 从环境变量加载
    // ==========================================================================
    Config config;
    config.load_from_env();

    // ==========================================================================
    // 3. 根据配置动态调整日志级别
    // ==========================================================================
    std::string log_level = config.get<std::string>(CONFIG_LOG_LEVEL, DEFAULT_LOG_LEVEL);
    spdlog::level::level_enum lvl = spdlog::level::from_str(log_level);
    LogManager::instance().set_level("default", lvl);

    // ==========================================================================
    // 4. 获取认证凭证 — 优先 ANTHROPIC_API_KEY，其次 ANTHROPIC_AUTH_TOKEN
    // ==========================================================================
    std::string api_key = config.get<std::string>(CONFIG_API_KEY, "");
    if (api_key.empty()) {
        api_key = config.get<std::string>(CONFIG_AUTH_TOKEN, "");
    }
    if (api_key.empty()) {
        // 严重错误：缺少 API 密钥，使用日志系统输出后退出
        LOG_CRITICAL("Missing API key. Set {} or {} environment variable.",
                      ENV_ANTHROPIC_API_KEY, ENV_ANTHROPIC_AUTH_TOKEN);
        LogManager::instance().shutdown();
        return 1;
    }

    LOG_INFO("AI Agent starting");

    // ==========================================================================
    // 5. 解析命令行参数
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
    // 6. 创建 Agent 并配置
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
    // 7. 加载内置工具
    // ==========================================================================
    agent.load_builtin_tools();

    LOG_INFO("Agent initialized with model: {}", model);

    // ==========================================================================
    // 8. 输出模型完整信息（同时输出到控制台和日志）
    // ==========================================================================
    std::string model_info = agent.get_model_info();
    LOG_INFO("Model info:\n{}", model_info);
    std::cout << model_info << std::endl;

    // ==========================================================================
    // 9. 一次性提问模式（如果提供了命令行提示词）
    // ==========================================================================
    if (!initial_prompt.empty()) {
        LOG_INFO("Running single prompt mode");
        std::string response = agent.run(utils::trim(initial_prompt));
        std::cout << response << std::endl;
    }

    // ==========================================================================
    // 10. 交互式 REPL 模式
    // ==========================================================================
    if (interactive) {
        LOG_INFO("Entering interactive REPL mode");
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

    LOG_INFO("AI Agent shutting down");
    LogManager::instance().shutdown();
    return 0;
}
