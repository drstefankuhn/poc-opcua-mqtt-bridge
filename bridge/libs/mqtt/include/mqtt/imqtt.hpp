#pragma once

#include <expected>
#include <string>

namespace mqtt {

/**
 * @brief Abstract MQTT client interface.
 *
 * @details
 * Defines the contract for connecting to an MQTT broker, publishing messages,
 * and disconnecting. Implementations must be move-only.
 */
class IMqtt {
  public:
    /** @brief Virtual destructor. */
    virtual ~IMqtt() noexcept = default;

    /** @brief Move constructor. */
    IMqtt(IMqtt &&other) noexcept = default;

    /** @brief Copying is not supported. */
    IMqtt(const IMqtt &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(IMqtt &&other) noexcept -> IMqtt & = default;

    /** @brief Copy assignment is not supported. */
    auto operator=(const IMqtt &) -> IMqtt & = delete;

    /**
     * @brief Connect to the MQTT broker.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto connect() noexcept -> std::expected<void, std::string> = 0;

    /**
     * @brief Publish a message to the configured topic.
     *
     * @param [in] message  Payload to publish.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto publish(const std::string &message) noexcept -> std::expected<void, std::string> = 0;

    /**
     * @brief Disconnect from the MQTT broker.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto disconnect() noexcept -> std::expected<void, std::string> = 0;

  protected:
    /** @brief Default constructor (for derived classes only). */
    IMqtt() = default;
};

} // namespace mqtt
