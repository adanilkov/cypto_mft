cmake_minimum_required(VERSION 3.10)

# Add unit test executable
add_executable(unit_tests
    main.cpp
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
)

# Add include directories
target_include_directories(unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Add test
add_test(NAME unit_tests COMMAND unit_tests) 