add_library(agent_core STATIC
    conversation.cpp
    tool_registry.cpp
    agent_loop.cpp
    api/http_client.cpp
    api/anthropic_client.cpp
    script/luau_engine.cpp
    script/script_tool.cpp
    process_engine/response_parser.cpp
    process_engine/message_formatter.cpp
)
target_include_directories(agent_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(agent_core PUBLIC agent_common httplib nlohmann_json Luau.VM Luau.Compiler)
