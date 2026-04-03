find_package(PahoMqttCpp REQUIRED)

add_library(mqtt SHARED
    src/paho_mqtt.cpp
)
add_library(mqtt::mqtt ALIAS mqtt)

target_include_directories(mqtt
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mqtt
    PUBLIC
        logger::logger
    PRIVATE
        paho-mqtt-cpp::paho-mqtt-cpp
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
