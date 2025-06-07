cmake_minimum_required(VERSION 3.10)

# Add unit test executable
add_executable(unit_tests
    main.cpp
    logger_test.cpp
    config_manager_test.cpp
    mock_exchange_test.cpp
    MockExchangeAdapter.cpp
)

# Link against required libraries
target_link_libraries(unit_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    spdlog::spdlog
    yaml-cpp
    prometheus-cpp-core
    prometheus-cpp-pull
    prometheus-cpp-push
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    nlohmann_json::nlohmann_json
    crypto_hft_core
    crypto_hft_infra
)

# Add include directories
target_include_directories(unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Copy test config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.yaml
    COPYONLY
)

# Add test
add_test(NAME unit_tests COMMAND unit_tests) 