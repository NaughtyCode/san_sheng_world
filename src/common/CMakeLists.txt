add_library(agent_common STATIC
    logger.cpp
    config.cpp
    utils.cpp
)
target_include_directories(agent_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(agent_common PUBLIC nlohmann_json)
