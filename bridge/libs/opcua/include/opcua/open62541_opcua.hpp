#pragma once

#include "iopcua.hpp"
#include "logger/ilogger.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>

namespace opcua {

namespace detail::internal {
struct Open62541OpcUaFactory;
}

/**
 * @brief IOpcUa implementation backed by the open62541 OPC UA library.
 *
 * @details
 * Uses `UA_Client` internally. Instances are created through the static
 * `make` factory and are move-only. Operations that fail return a descriptive
 * error string instead of throwing. A moved-from instance rejects all
 * operations with an error.
 */
class Open62541OpcUa : public IOpcUa {
  public:
    /** @brief Destructor. */
    ~Open62541OpcUa() noexcept override;

    /** @brief Move constructor. */
    Open62541OpcUa(Open62541OpcUa &&other) noexcept;

    /** @brief Copying is not supported. */
    Open62541OpcUa(const Open62541OpcUa &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(Open62541OpcUa &&other) noexcept -> Open62541OpcUa &;

    /** @brief Copy assignment is not supported. */
    auto operator=(const Open62541OpcUa &) -> Open62541OpcUa & = delete;

    /**
     * @brief Create an Open62541OpcUa instance.
     *
     * @param [in] logger  Logger for diagnostic output.
     * @param [in] host    Server hostname.
     * @param [in] port    Server TCP port.
     * @param [in] path    OPC UA namespace path (e.g. `"/freeopcua/server/"`).
     *
     * @return A valid Open62541OpcUa on success, or a string describing the error.
     */
    [[nodiscard]] static auto make(logger::ILogger &logger, const std::string &host, uint16_t port,
                                   const std::string &path) -> std::expected<Open62541OpcUa, std::string>;

    /**
     * @brief Connect to the OPC UA server.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto connect() noexcept -> std::expected<void, std::string> override;

    /**
     * @brief Read current values for the given node paths.
     *
     * @param [in,out] pathValues  Path-value pairs whose values are populated on success.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto read(std::span<PathValue> pathValues) noexcept -> std::expected<void, std::string> override;

    /**
     * @brief Disconnect from the OPC UA server.
     *
     * @return An empty expected on success, or a string describing the error.
     */
    auto disconnect() noexcept -> std::expected<void, std::string> override;

  private:
    Open62541OpcUa() = default;
    struct Open62541OpcUaImpl;
    std::unique_ptr<Open62541OpcUaImpl> _impl;

    friend struct detail::internal::Open62541OpcUaFactory;
};

} // namespace opcua
