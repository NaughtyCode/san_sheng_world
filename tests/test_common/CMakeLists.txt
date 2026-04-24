add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger PRIVATE agent_common gtest_main)
add_test(NAME test_logger COMMAND test_logger)

add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE agent_common gtest_main)
add_test(NAME test_config COMMAND test_config)
