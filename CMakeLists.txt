cmake_minimum_required(VERSION 3.20)
project(crypto-hft LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Enable LTO (Link Time Optimization)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Include FetchContent for dependency management
include(FetchContent)

# Fetch and configure GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Fetch and configure spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# Fetch and configure yaml-cpp
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# Fetch and configure nlohmann_json
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Set include directories for the project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/exchanges)
add_subdirectory(src/infra)
add_subdirectory(src/models)
add_subdirectory(src/utils)
add_subdirectory(src/tests)

# Create main executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Set warning flags only for our project
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
    target_compile_options(core PRIVATE -Wall -Wextra -Werror)
    target_compile_options(exchanges PRIVATE -Wall -Wextra -Werror)
    target_compile_options(infra PRIVATE -Wall -Wextra -Werror)
    target_compile_options(models PRIVATE -Wall -Wextra -Werror)
    target_compile_options(utils PRIVATE -Wall -Wextra -Werror)
    target_compile_options(infra_tests PRIVATE -Wall -Wextra -Werror)
endif()

# Link against all subdirectories
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    core
    exchanges
    infra
    models
    utils
    spdlog::spdlog
    yaml-cpp
    nlohmann_json::nlohmann_json
)
