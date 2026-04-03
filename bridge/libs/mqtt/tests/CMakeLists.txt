find_package(Catch2 REQUIRED)

add_executable(mqtt_tests
    paho_mqtt_test.cpp
)

target_include_directories(mqtt_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(mqtt_tests
    PRIVATE
        mqtt::mqtt
        Catch2::Catch2WithMain
)

add_test(NAME mqtt_tests COMMAND mqtt_tests)
