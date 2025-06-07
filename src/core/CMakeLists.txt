# Core library
add_library(crypto_hft_core
    MarketDataEngine.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_core
    PUBLIC
    crypto_hft_utils
    spdlog::spdlog
    yaml-cpp
    prometheus-cpp::core
    OpenSSL::SSL
    OpenSSL::Crypto
    ${Boost_LIBRARIES}
    Threads::Threads
)

# Include directories
target_include_directories(crypto_hft_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
) 