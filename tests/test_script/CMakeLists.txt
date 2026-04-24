add_executable(test_luau_engine test_luau_engine.cpp)
target_link_libraries(test_luau_engine PRIVATE agent_core agent_common gtest_main Luau.VM Luau.Compiler)
add_test(NAME test_luau_engine COMMAND test_luau_engine)
