enable_testing()

# Set test include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(infra_tests
    config_manager_test.cpp
    logger_test.cpp
)

target_link_libraries(infra_tests
    PRIVATE
    infra
    GTest::gtest_main
    GTest::gmock_main
    yaml-cpp
    spdlog::spdlog
)

# Copy test config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/test_config.yaml
    COPYONLY
)

# Add tests to CTest
add_test(NAME infra_tests COMMAND infra_tests) 