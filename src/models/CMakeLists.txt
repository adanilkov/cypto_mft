add_library(models
    models.cpp
)

target_include_directories(models
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(models
    PUBLIC
    core
    spdlog::spdlog
) 