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

# cpp-linenoise (Header-only line editing library)
add_library(linenoise INTERFACE)
target_include_directories(linenoise INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cpp-linenoise)

# Boost.Nowide — standalone console I/O with UTF-8 support (no full Boost required)
add_library(nowide_console STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/src/iostream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/src/console_buffer.cpp
)
target_include_directories(nowide_console PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/standalone_boost   # Boost shim headers (must come first)
    ${CMAKE_CURRENT_SOURCE_DIR}/nowide/include             # Nowide headers
)
target_compile_definitions(nowide_console PUBLIC BOOST_NOWIDE_NO_LIB)
target_compile_features(nowide_console PUBLIC cxx_std_11)
