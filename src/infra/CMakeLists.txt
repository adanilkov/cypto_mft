add_library(infra
    ConfigManager.cpp
    Logger.cpp
)

target_include_directories(infra
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(infra
    PUBLIC
    yaml-cpp
    spdlog::spdlog
) 