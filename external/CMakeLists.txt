# cpp-httplib (Header-only) — requires OpenSSL for HTTPS
set(OPENSSL_ROOT_DIR "${OPENSSL_ROOT_DIR}" CACHE PATH "OpenSSL root directory")
find_package(OpenSSL REQUIRED)
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp-httplib)
target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
target_link_libraries(httplib INTERFACE OpenSSL::SSL OpenSSL::Crypto)

# nlohmann/json (Header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/include)

# spdlog (Header-only, compiled library for faster builds)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/spdlog EXCLUDE_FROM_ALL)

# Luau
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/luau EXCLUDE_FROM_ALL)
