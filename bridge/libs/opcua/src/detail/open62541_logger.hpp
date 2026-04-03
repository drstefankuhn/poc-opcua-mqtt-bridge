#pragma once

#include "logger/ilogger.hpp"

#include <open62541/plugin/log.h>

namespace opcua::detail {

/**
 * @brief Create a `UA_Logger` that forwards open62541 log output to an ILogger.
 *
 * @details
 * The returned `UA_Logger` stores a pointer to @p ilogger in its context
 * field. The caller must ensure @p ilogger outlives the returned logger.
 *
 * @param [in] ilogger  Target logger that receives the forwarded messages.
 *
 * @return A `UA_Logger` bridge instance.
 */
[[nodiscard]] auto make_ua_logger_bridge(logger::ILogger &ilogger) -> UA_Logger;

} // namespace opcua::detail
