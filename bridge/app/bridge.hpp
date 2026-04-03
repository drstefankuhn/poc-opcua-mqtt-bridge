#pragma once

#include "mqtt/imqtt.hpp"
#include "opcua/iopcua.hpp"

#include <expected>
#include <span>
#include <string_view>
#include <vector>

namespace bridge {

/**
 * @brief Bridges OPC UA node reads to MQTT message publishes.
 *
 * @details
 * Connects to an OPC UA server, reads the configured node paths, serializes
 * the results as JSON, and publishes the payload to an MQTT broker. Both
 * client dependencies are injected as interfaces.
 */
class Bridge {
  public:
    /**
     * @brief Construct a Bridge.
     *
     * @param [in] opcua        OPC UA client used for reading node values.
     * @param [in] mqtt         MQTT client used for publishing messages.
     * @param [in] opcua_paths  OPC UA node paths to read.
     */
    Bridge(opcua::IOpcUa &opcua, mqtt::IMqtt &mqtt, std::span<const std::string_view> opcua_paths);

    /** @brief Destructor. */
    ~Bridge() noexcept = default;

    /** @brief Move constructor. */
    Bridge(Bridge &&other) noexcept = default;

    /** @brief Copying is not supported. */
    Bridge(const Bridge &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(Bridge &&other) noexcept -> Bridge & = default;

    /** @brief Copy assignment is not supported. */
    auto operator=(const Bridge &) -> Bridge & = delete;

    /**
     * @brief Execute one bridge cycle: read from OPC UA, publish to MQTT.
     *
     * @details
     * Performs connect, read, disconnect on the OPC UA client, then connect,
     * publish, disconnect on the MQTT client. Disconnect is always attempted
     * even when earlier operations fail.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] auto run() noexcept -> std::expected<void, std::string>;

  private:
    opcua::IOpcUa *_opcua;
    mqtt::IMqtt *_mqtt;
    std::vector<std::string_view> _opcua_paths;
};

} // namespace bridge
