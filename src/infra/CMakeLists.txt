# Infrastructure library
add_library(crypto_hft_infra
    ConfigManager.cpp
    Logger.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_infra
    PRIVATE
    crypto_hft_utils
    spdlog::spdlog
    yaml-cpp
    prometheus-cpp::core
    prometheus-cpp::pull
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Include directories
target_include_directories(crypto_hft_infra
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
) 