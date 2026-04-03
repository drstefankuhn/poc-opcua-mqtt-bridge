find_package(Catch2 REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(bridge_tests
    bridge_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../bridge.cpp
)

target_include_directories(bridge_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/opcua/src
)

target_link_libraries(bridge_tests
    PRIVATE
        mqtt::mqtt
        opcua::opcua
        nlohmann_json::nlohmann_json
        Catch2::Catch2WithMain
)

add_test(NAME bridge_tests COMMAND bridge_tests)
