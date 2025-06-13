add_library(crypto_hft_models
    SignalGenerator.cpp
)

target_link_libraries(crypto_hft_models
    PRIVATE
    crypto_hft_utils
    crypto_hft_core
    spdlog::spdlog
    yaml-cpp
)

target_include_directories(crypto_hft_models
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
) 