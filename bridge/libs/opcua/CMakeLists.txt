find_package(open62541 REQUIRED)

add_library(opcua SHARED
    src/open62541_opcua.cpp
    src/path_value.cpp
    src/detail/open62541_logger.cpp
    src/detail/open62541_value_reader.cpp
)
add_library(opcua::opcua ALIAS opcua)

target_include_directories(opcua
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(opcua
    PUBLIC
        logger::logger
    PRIVATE
        open62541::open62541
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
