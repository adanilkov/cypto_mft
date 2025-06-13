cmake_minimum_required(VERSION 3.10)

# Add integration test executable
add_executable(integration_tests
    main.cpp
)

# Link against required libraries
target_link_libraries(integration_tests
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
)

# Add include directories
target_include_directories(integration_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Add test
add_test(NAME integration_tests COMMAND integration_tests)

# Find required packages
find_package(GTest REQUIRED)
find_package(Boost 1.74.0 REQUIRED COMPONENTS system thread)

add_executable(order_book_pipeline_test order_book_pipeline_test.cpp)

target_link_libraries(order_book_pipeline_test
    PRIVATE
    crypto_hft_core
    crypto_hft_exchanges
    crypto_hft_infra
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    ${Boost_THREAD_LIBRARY}
    pthread
)

add_test(NAME order_book_pipeline_test COMMAND order_book_pipeline_test) 