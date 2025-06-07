add_library(exchanges
    exchanges.cpp
)

target_include_directories(exchanges
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(exchanges
    PUBLIC
    core
    spdlog::spdlog
) 