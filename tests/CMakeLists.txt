cmake_minimum_required(VERSION 3.10)
project(crypto-hft-tests)

# Enable testing
enable_testing()

# Find required packages
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(prometheus-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Add test executables
add_subdirectory(unit)
add_subdirectory(integration) 