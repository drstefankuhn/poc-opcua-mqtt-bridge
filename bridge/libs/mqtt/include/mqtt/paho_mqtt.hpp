#pragma once

#include "logger/ilogger.hpp"
#include "mqtt/imqtt.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <string>

namespace mqtt {

namespace detail::internal {
struct PahoMqttFactory;
}

/**
 * @brief IMqtt implementation backed by the Eclipse Paho C++ MQTT library.
 *
 * @details
 * Uses `mqtt::async_client` internally. Instances are created through the
 * static `make` factory and are move-only. Operations that fail return a
 * descriptive error string instead of throwing. A moved-from instance rejects
 * all operations with an error.
 */
class PahoMqtt : public IMqtt {
  public:
    /** @brief Destructor. */
    ~PahoMqtt() noexcept override;

    /** @brief Move constructor. */
    PahoMqtt(PahoMqtt &&other) noexcept;

    /** @brief Copying is not supported. */
    PahoMqtt(const PahoMqtt &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(PahoMqtt &&other) noexcept -> PahoMqtt &;

    /** @brief Copy assignment is not supported. */
    auto operator=(const PahoMqtt &) -> PahoMqtt & = delete;

    /**
     * @brief Create a PahoMqtt instance.
     *
     * @param [in] logger  Logger for diagnostic output.
     * @param [in] host    Broker hostname.
     * @param [in] port    Broker TCP port.
     * @param [in] topic   MQTT topic to publish to.
     *
     * @return A valid PahoMqtt on success, or a string describing the error.
     */
    [[nodiscard]] static auto make(logger::ILogger &logger, const std::string &host, uint16_t port, std::string topic)
        -> std::expected<PahoMqtt, std::string>;

    /**
     * @brief Connect to the MQTT broker.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto connect() noexcept -> std::expected<void, std::string> override;

    /**
     * @brief Publish a message to the configured topic.
     *
     * @param [in] message  Payload to publish.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto publish(const std::string &message) noexcept -> std::expected<void, std::string> override;

    /**
     * @brief Disconnect from the MQTT broker.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto disconnect() noexcept -> std::expected<void, std::string> override;

  private:
    PahoMqtt() = default;
    struct PahoMqttImpl;
    std::unique_ptr<PahoMqttImpl> _impl;

    friend struct detail::internal::PahoMqttFactory;
};

} // namespace mqtt
