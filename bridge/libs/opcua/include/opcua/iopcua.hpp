#pragma once

#include "path_value.hpp"

#include <expected>
#include <span>
#include <string>

namespace opcua {

/**
 * @brief Abstract OPC UA client interface.
 *
 * @details
 * Defines the contract for connecting to an OPC UA server, reading node
 * values, and disconnecting. Implementations must be move-only.
 */
class IOpcUa {
  public:
    /** @brief Virtual destructor. */
    virtual ~IOpcUa() noexcept = default;

    /** @brief Move constructor. */
    IOpcUa(IOpcUa &&other) noexcept = default;

    /** @brief Copying is not supported. */
    IOpcUa(const IOpcUa &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(IOpcUa &&other) noexcept -> IOpcUa & = default;

    /** @brief Copy assignment is not supported. */
    auto operator=(const IOpcUa &) -> IOpcUa & = delete;

    /**
     * @brief Connect to the OPC UA server.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto connect() noexcept -> std::expected<void, std::string> = 0;

    /**
     * @brief Read current values for the given node paths.
     *
     * @param [in,out] pathValues  Path-value pairs whose values are populated on success.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto read(std::span<PathValue> pathValues) noexcept -> std::expected<void, std::string> = 0;

    /**
     * @brief Disconnect from the OPC UA server.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    [[nodiscard]] virtual auto disconnect() noexcept -> std::expected<void, std::string> = 0;

  protected:
    /** @brief Default constructor (for derived classes only). */
    IOpcUa() = default;
};

} // namespace opcua
