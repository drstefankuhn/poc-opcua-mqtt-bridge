#pragma once

#include "opcua/iopcua.hpp"
#include "opcua/open62541_opcua.hpp"

#include <open62541/client.h>

#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace opcua::detail::internal {

/**
 * @brief Indirection table for open62541 and local integration calls used by Open62541OpcUa.
 *
 * @details
 * Production code passes default ops; tests inject stubs via
 * Open62541OpcUaFactory::make_with_ops(). This seam can be extended with
 * additional UA client lifecycle/configuration calls if deeper unit-level
 * isolation is needed.
 */
struct Open62541OpcUaOps {
    /**
     * @brief Connect to an OPC UA server.
     *
     * @param [in] client        Pointer to the open62541 client.
     * @param [in] endpoint_url  Server endpoint URL.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*connect)(UA_Client *client, const char *endpoint_url);

    /**
     * @brief Disconnect from the OPC UA server.
     *
     * @param [in] client  Pointer to the open62541 client.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*disconnect)(UA_Client *client);

    /**
     * @brief Return a human-readable name for a status code.
     *
     * @param [in] code  OPC UA status code.
     *
     * @return Null-terminated status code name string.
     */
    const char *(*status_code_name)(UA_StatusCode code);

    /**
     * @brief Read the value of an OPC UA node identified by path.
     *
     * @param [in] client  Pointer to the open62541 client.
     * @param [in] path    Slash-separated node path (e.g. `"Demo/Counter"`).
     *
     * @return The value if the node was found, or `std::nullopt`.
     */
    std::optional<PathValue::Value> (*read_value)(UA_Client *client, std::string_view path);

    /**
     * @brief Write a value into a PathValue instance.
     *
     * @param [in,out] path_value  Target PathValue whose value is set.
     * @param [in]     value       Value to assign.
     */
    void (*set_path_value)(PathValue &path_value, const PathValue::Value &value);
};

/**
 * @brief Internal factory that creates Open62541OpcUa instances with a given ops table.
 *
 * @details
 * Used by Open62541OpcUa::make() with production ops and by tests with stub ops.
 */
struct Open62541OpcUaFactory {
    /**
     * @brief Create an Open62541OpcUa instance with the given ops table.
     *
     * @param [in] ops     Indirection table for OPC UA client calls.
     * @param [in] logger  Logger for diagnostic output.
     * @param [in] host    Server hostname.
     * @param [in] port    Server TCP port.
     * @param [in] path    OPC UA namespace path.
     *
     * @return A valid Open62541OpcUa on success, or a string describing the error.
     */
    static auto make_with_ops(Open62541OpcUaOps ops, logger::ILogger &logger, const std::string &host, uint16_t port,
                              const std::string &path) -> std::expected<Open62541OpcUa, std::string>;
};

} // namespace opcua::detail::internal
