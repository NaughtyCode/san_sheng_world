# cpp-httplib (Header-only)
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib)

# nlohmann/json (Header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/include)

# Luau
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/luau EXCLUDE_FROM_ALL)
