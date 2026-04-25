add_library(agent_common STATIC
    constants.cpp
    logger.cpp
    ../log/log_manager.cpp
    config.cpp
    utils.cpp
)
target_include_directories(agent_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/log
)
target_link_libraries(agent_common PUBLIC nlohmann_json spdlog::spdlog)
