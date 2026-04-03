#pragma once

#include "logger/ilogger.hpp"

#include <open62541/plugin/log.h>

#include <cstdarg>
#include <string>

namespace opcua::detail::internal {

/**
 * @brief Convert a `UA_LogCategory` to its human-readable name.
 *
 * @param [in] category  open62541 log category.
 *
 * @return Category name (e.g. `"CLIENT"`, `"NETWORK"`), or `"UNKNOWN"`.
 */
[[nodiscard]] auto category_to_string(UA_LogCategory category) -> std::string;

/**
 * @brief Format a `printf`-style message with a `va_list` into a string.
 *
 * @param [in] msg   Format string.
 * @param [in] args  Variable argument list matching @p msg.
 *
 * @return The formatted string, or the raw @p msg on formatting failure.
 */
[[nodiscard]] auto msg_args_to_string(const char *msg, va_list args) -> std::string;

/**
 * @brief Route a log line to the appropriate ILogger severity method.
 *
 * @param [in] ilogger  Target logger.
 * @param [in] level    open62541 log level that selects the ILogger method.
 * @param [in] line     Pre-formatted log line to emit.
 */
auto write_to_ilogger(logger::ILogger &ilogger, UA_LogLevel level, const std::string &line) -> void;

} // namespace opcua::detail::internal
