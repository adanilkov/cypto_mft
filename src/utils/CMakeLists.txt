# Utils library
add_library(crypto_hft_utils
    # Types.cpp
    # Constants.cpp
    # Utils.cpp
)

# Link dependencies
target_link_libraries(crypto_hft_utils
    PRIVATE
    spdlog::spdlog
    yaml-cpp
    nlohmann_json::nlohmann_json
)

# Include directories
target_include_directories(crypto_hft_utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
) 