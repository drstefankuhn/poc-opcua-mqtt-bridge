#pragma once

#include "opcua/iopcua.hpp"

namespace opcua::detail::internal {

/**
 * @brief Grants write access to PathValue's private value member.
 *
 * @details
 * Used by IOpcUa implementations to populate PathValue instances after
 * reading node values from the server.
 */
struct PathValueMutator {
    /**
     * @brief Set the value of a PathValue.
     *
     * @param [in,out] path_value  Target PathValue whose value is set.
     * @param [in]     value       Value to assign.
     */
    static auto set_path_value(PathValue &path_value, const PathValue::Value &value) -> void;
};

} // namespace opcua::detail::internal
