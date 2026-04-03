#pragma once

#include "mqtt/paho_mqtt.hpp"

#include <expected>
#include <string>

namespace mqtt::detail::internal {

/**
 * @brief Indirection table for Paho MQTT client calls used by PahoMqtt.
 *
 * @details
 * Production passes default ops; tests inject stubs via
 * PahoMqttFactory::make_with_ops(). Ops throw on failure; PahoMqtt methods
 * catch and convert to `std::expected`.
 */
struct PahoMqttOps {
    /**
     * @brief Connect to the broker.
     *
     * @param [in] client  Opaque pointer to the underlying `mqtt::async_client`.
     * @param [in] broker  Broker URI (e.g. `"tcp://localhost:1883"`).
     */
    void (*connect)(void *client, const std::string &broker);

    /**
     * @brief Publish a message on a topic.
     *
     * @param [in] client   Opaque pointer to the underlying `mqtt::async_client`.
     * @param [in] topic    MQTT topic to publish to.
     * @param [in] message  Payload to publish.
     */
    void (*publish)(void *client, const std::string &topic, const std::string &message);

    /**
     * @brief Disconnect from the broker.
     *
     * @param [in] client  Opaque pointer to the underlying `mqtt::async_client`.
     */
    void (*disconnect)(void *client);
};

/**
 * @brief Internal factory that creates PahoMqtt instances with a given ops table.
 *
 * @details
 * Used by PahoMqtt::make() with production ops and by tests with stub ops.
 */
struct PahoMqttFactory {
    /**
     * @brief Create a PahoMqtt instance with the given ops table.
     *
     * @param [in] ops     Indirection table for MQTT client calls.
     * @param [in] logger  Logger for diagnostic output.
     * @param [in] host    Broker hostname.
     * @param [in] port    Broker TCP port.
     * @param [in] topic   MQTT topic to publish to.
     *
     * @return A valid PahoMqtt on success, or a string describing the error.
     */
    static auto make_with_ops(PahoMqttOps ops, logger::ILogger &logger, const std::string &host, uint16_t port,
                              std::string topic) -> std::expected<PahoMqtt, std::string>;
};

} // namespace mqtt::detail::internal
