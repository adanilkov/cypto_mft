cmake_minimum_required(VERSION 3.20)
project(crypto-hft LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Include FetchContent
include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# Find required packages
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(prometheus-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(websocketpp REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SPDLOG_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    ${PROMETHEUS_CPP_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Add subdirectories
add_subdirectory(src)

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    crypto_hft_core
    crypto_hft_models
    crypto_hft_infra
    crypto_hft_exchanges
    crypto_hft_utils
    spdlog::spdlog
    yaml-cpp
    prometheus-cpp::core
    prometheus-cpp::pull
    OpenSSL::SSL
    OpenSSL::Crypto
    websocketpp
    ${Boost_LIBRARIES}
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install configuration files
install(DIRECTORY configs/
    DESTINATION etc/${PROJECT_NAME}
)

# Enable testing
enable_testing()
add_subdirectory(tests)
