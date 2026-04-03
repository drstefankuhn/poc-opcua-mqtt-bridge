#include "bridge.hpp"
#include "logger/stdout_logger.hpp"
#include "mqtt/paho_mqtt.hpp"
#include "opcua/open62541_opcua.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

/**
 * @brief Read an environment variable or return a fallback value.
 *
 * @param [in] name      Environment variable name.
 * @param [in] fallback  Value returned when the variable is not set.
 *
 * @return The environment variable's value, or @p fallback.
 */
[[nodiscard]] auto get_env_or_default(const char *name, const char *fallback) -> std::string {
    if (const char *value = std::getenv(name)) {
        return value;
    }
    return fallback;
}

/**
 * @brief Parse a string as a TCP port number.
 *
 * @param [in] port  String representation of the port (e.g. `"1883"`).
 *
 * @return The port as `uint16_t` on success, or a string describing the error.
 */
[[nodiscard]] auto to_port(const std::string &port) -> std::expected<std::uint16_t, std::string> {
    try {
        auto const port_number = std::stoi(port);
        if (port_number < 1 || port_number > std::numeric_limits<std::uint16_t>::max()) {
            return std::unexpected("Port number out of range: " + port);
        }
        return static_cast<std::uint16_t>(port_number);
    } catch (const std::exception &e) {
        return std::unexpected("Failed to convert port to number: " + std::string(e.what()));
    }
}

} // namespace

/**
 * @brief Application entry point.
 *
 * @details
 * Reads configuration from environment variables, creates OPC UA and MQTT
 * clients, and runs one bridge cycle.
 *
 * @retval 0  Success.
 * @retval 1  Error (details printed to `std::cerr`).
 */
auto main() -> int {
    // Read environment variables
    auto const opcua_host = get_env_or_default("OPCUA_HOST", "opcua");
    auto const opcua_port = to_port(get_env_or_default("OPCUA_PORT", "4840"));
    if (!opcua_port) {
        std::cerr << opcua_port.error() << '\n';
        return 1;
    }
    auto const opcua_path = get_env_or_default("OPCUA_PATH", "/freeopcua/server/");
    auto const mqtt_host = get_env_or_default("MQTT_HOST", "mqtt");
    auto const mqtt_port = to_port(get_env_or_default("MQTT_PORT", "1883"));
    if (!mqtt_port) {
        std::cerr << mqtt_port.error() << '\n';
        return 1;
    }
    auto const mqtt_topic = get_env_or_default("MQTT_TOPIC", "outputs");

    // Create concrete OPC UA client
    auto opcua_logger = logger::StdoutLogger("OPC UA");
    auto opcua = opcua::Open62541OpcUa::make(opcua_logger, opcua_host, opcua_port.value(), opcua_path);

    // Create OPC UA paths that will be bridged
    auto const opcua_paths = std::to_array<std::string_view>(
        {"Demo/Counter", "Demo/Temperature", "Demo/Running", "Demo/MachineState", "Demo/Pressure", "Demo/Setpoint"});

    // Create concrete MQTT client
    auto mqtt_logger = logger::StdoutLogger("MQTT");
    auto mqtt = mqtt::PahoMqtt::make(mqtt_logger, mqtt_host, mqtt_port.value(), mqtt_topic);

    if (opcua && mqtt) {

        // Create bridge
        auto bridge = bridge::Bridge(opcua.value(), mqtt.value(), opcua_paths);

        // Run bridge once
        if (auto result = bridge.run(); result) {
            std::cout << "Bridge run successfully\n";
        } else {
            std::cerr << "Bridge run failed: " << result.error() << '\n';
            return 1;
        }
    } else {
        std::cerr << "Failed to create OPC UA or MQTT client\n";
        return 1;
    }

    return 0;
}
