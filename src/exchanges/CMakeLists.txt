# Exchanges library
add_library(crypto_hft_exchanges
    # BinanceAdapter.cpp
    # BybitAdapter.cpp
    # OKXAdapter.cpp
    # CoinbaseAdapter.cpp
    # KrakenAdapter.cpp
    # ExchangeUtils.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_exchanges
    PRIVATE
    crypto_hft_utils
    crypto_hft_core
    spdlog::spdlog
    yaml-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    websocketpp
    Boost::system
)

# Include directories
target_include_directories(crypto_hft_exchanges
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
) 