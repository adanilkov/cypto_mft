# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.22.1)

# We name the project and the target for the ExternalProject_Add() call
# to something that will highlight to the user what we are working on if
# something goes wrong and an error message is produced.

project(jwt-cpp-populate NONE)



include(ExternalProject)
ExternalProject_Add(jwt-cpp-populate
                     "UPDATE_DISCONNECTED" "False" "URL" "https://github.com/Thalhammer/jwt-cpp/archive/refs/tags/v0.7.1.tar.gz"
                    SOURCE_DIR          "/home/adanilkovas/projects/crypto/crypto-hft/build/_deps/jwt-cpp-src"
                    BINARY_DIR          "/home/adanilkovas/projects/crypto/crypto-hft/build/_deps/jwt-cpp-build"
                    CONFIGURE_COMMAND   ""
                    BUILD_COMMAND       ""
                    INSTALL_COMMAND     ""
                    TEST_COMMAND        ""
                    USES_TERMINAL_DOWNLOAD  YES
                    USES_TERMINAL_UPDATE    YES
)


