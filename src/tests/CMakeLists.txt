enable_testing()

# Set test include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Find GTest package
find_package(GTest REQUIRED)

# Test executables
# add_executable(core_tests
#     CoreTests.cpp
# )

# add_executable(exchange_tests
#     ExchangeAdapterTests.cpp
# )

# add_executable(strategy_tests
#     StrategyTests.cpp
# )

# Test executable
add_executable(crypto_hft_tests
    config_manager_test.cpp
    logger_test.cpp
    mock_exchange_test.cpp
)

# Link test executables
# target_link_libraries(core_tests
#     PRIVATE
#     crypto_hft_core
#     crypto_hft_utils
#     GTest::GTest
#     GTest::Main
# )

# target_link_libraries(exchange_tests
#     PRIVATE
#     crypto_hft_exchanges
#     crypto_hft_utils
#     GTest::GTest
#     GTest::Main
# )

# target_link_libraries(strategy_tests
#     PRIVATE
#     crypto_hft_models
#     crypto_hft_utils
#     GTest::GTest
#     GTest::Main
# )

# Link dependencies
target_link_libraries(crypto_hft_tests
    PRIVATE
    crypto_hft_core
    crypto_hft_exchanges
    crypto_hft_models
    crypto_hft_infra
    crypto_hft_utils
    gtest
    gtest_main
    spdlog::spdlog
    yaml-cpp
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Include directories
target_include_directories(crypto_hft_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Copy test config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.yaml
    COPYONLY
)

# Add tests
# add_test(NAME core_tests COMMAND core_tests)
# add_test(NAME exchange_tests COMMAND exchange_tests)
# add_test(NAME strategy_tests COMMAND strategy_tests)
add_test(NAME crypto_hft_tests COMMAND crypto_hft_tests) 