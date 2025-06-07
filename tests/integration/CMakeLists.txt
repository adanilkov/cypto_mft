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