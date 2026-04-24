#include <iostream>
#include <string>
#include <cstdlib>

#include "logger.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "agent_loop.hpp"

using namespace agent;

int main(int argc, char* argv[]) {
    // Initialize config
    Config config;
    config.load_from_env();

    // Set up logging
    Logger& log = Logger::instance();
    std::string log_level = config.get<std::string>("log_level", "info");
    std::string log_file = config.get<std::string>("log_file", "");

    if (log_level == "debug") log.set_level(LogLevel::Debug);
    else if (log_level == "warning") log.set_level(LogLevel::Warning);
    else if (log_level == "error") log.set_level(LogLevel::Error);
    else log.set_level(LogLevel::Info);

    if (!log_file.empty()) {
        log.set_file(log_file);
    }

    log.info("AI Agent starting");

    // Get API key
    std::string api_key = config.get<std::string>("api_key", "");
    if (api_key.empty()) {
        std::cerr << "Error: ANTHROPIC_API_KEY not set. Export it or set in config." << std::endl;
        return 1;
    }

    // Parse command-line arguments
    bool interactive = true;
    std::string initial_prompt;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-interactive" || arg == "-n") {
            interactive = false;
        } else if (arg == "--model" || arg == "-m") {
            if (i + 1 < argc) {
                config.set("model", argv[++i]);
            }
        } else {
            initial_prompt += arg;
            if (i + 1 < argc) initial_prompt += " ";
        }
    }

    // Create agent
    AgentLoop agent;
    agent.set_api_key(api_key);

    std::string model = config.get<std::string>("model", "claude-sonnet-4-6");
    agent.set_model(model);

    // Load built-in tools
    agent.load_builtin_tools();

    log.info("Agent initialized with model: %s", model.c_str());

    // Print model info
    std::cout << agent.get_model_info() << std::endl;

    // Run with initial prompt if provided
    if (!initial_prompt.empty()) {
        std::string response = agent.run(utils::trim(initial_prompt));
        std::cout << response << std::endl;
    }

    // Interactive loop
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
