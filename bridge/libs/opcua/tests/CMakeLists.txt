find_package(Catch2 REQUIRED)

add_executable(opcua_tests
    path_value_test.cpp
    open62541_value_reader_test.cpp
    open62541_logger_test.cpp
    open62541_opcua_test.cpp
)

target_include_directories(opcua_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(opcua_tests
    PRIVATE
        opcua::opcua
        open62541::open62541
        Catch2::Catch2WithMain
)

add_test(NAME opcua_tests COMMAND opcua_tests)
