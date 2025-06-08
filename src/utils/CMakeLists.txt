# Utils library
add_library(crypto_hft_utils
    utils.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_utils
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
)

# Include directories
target_include_directories(crypto_hft_utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/jwt-cpp-src/include
) 