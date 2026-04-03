#pragma once

#include "opcua/path_value.hpp"

#include <open62541/client.h>

#include <optional>
#include <string_view>

namespace opcua::detail {

/**
 * @brief Read the value of an OPC UA node identified by a slash-separated path.
 *
 * @details
 * Resolves @p path starting from the Objects folder, reads the node value,
 * and converts it to a PathValue::Value variant. Uses the default
 * open62541 client API internally.
 *
 * @param [in] client  Connected open62541 client.
 * @param [in] path    Slash-separated node path (e.g. `"Demo/Counter"`).
 *
 * @return The value if the node was found and readable, or `std::nullopt`.
 */
[[nodiscard]] auto read_value(UA_Client *client, std::string_view path) -> std::optional<PathValue::Value>;

} // namespace opcua::detail
