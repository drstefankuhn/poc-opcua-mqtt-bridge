add_library(logger SHARED
    src/stdout_logger.cpp
)
add_library(logger::logger ALIAS logger)

target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
