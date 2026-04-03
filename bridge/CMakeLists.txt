cmake_minimum_required(VERSION 3.23)
project(poc-opcua-mqtt-bridge LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(libs/logger)
add_subdirectory(libs/mqtt)
add_subdirectory(libs/opcua)

find_package(nlohmann_json REQUIRED)

add_executable(poc-opcua-mqtt-bridge
    app/main.cpp
    app/bridge.cpp
)

target_link_libraries(poc-opcua-mqtt-bridge
    PUBLIC
        mqtt::mqtt
        opcua::opcua
    PRIVATE
        nlohmann_json::nlohmann_json
        logger::logger
)

# Copies compile_commands.json from the active preset binary dir into bridge/
# so clangd does not need a hard-coded build/Release or Debug path.
add_custom_target(
    sync_compile_commands
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Sync compile_commands.json for clangd (current CMake/Conan preset)"
    VERBATIM
)
add_dependencies(poc-opcua-mqtt-bridge sync_compile_commands)

if (BUILD_TESTING)
    add_subdirectory(app/tests)
endif()
