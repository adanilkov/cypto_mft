# Core library
add_library(crypto_hft_core
    MarketDataEngine.cpp
    OrderBook.cpp
    TradingSystem.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_core
    PUBLIC
    crypto_hft_utils
    crypto_hft_infra
    crypto_hft_exchanges
    spdlog::spdlog
    yaml-cpp
    prometheus-cpp::core
    OpenSSL::SSL
    OpenSSL::Crypto
    ${Boost_LIBRARIES}
    Threads::Threads
    concurrentqueue
)

# Include directories
target_include_directories(crypto_hft_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external/concurrentqueue
    ${CMAKE_SOURCE_DIR}/src/infra
    ${CMAKE_SOURCE_DIR}/src/exchanges
) 