#include "open62541_logger.hpp"
#include "open62541_logger_internal.hpp"

#include <open62541/types.h>

#include <algorithm>
#include <experimental/scope>
#include <span>
#include <sstream>
#include <string>

namespace opcua::detail {

namespace internal {

[[nodiscard]] auto category_to_string(UA_LogCategory category) -> std::string {
    switch (category) {
    case UA_LOGCATEGORY_NETWORK:
        return "NETWORK";
    case UA_LOGCATEGORY_SECURECHANNEL:
        return "SECURECHANNEL";
    case UA_LOGCATEGORY_SESSION:
        return "SESSION";
    case UA_LOGCATEGORY_SERVER:
        return "SERVER";
    case UA_LOGCATEGORY_CLIENT:
        return "CLIENT";
    case UA_LOGCATEGORY_APPLICATION:
        return "APPLICATION";
    case UA_LOGCATEGORY_SECURITY:
        return "SECURITY";
    case UA_LOGCATEGORY_EVENTLOOP:
        return "EVENTLOOP";
    case UA_LOGCATEGORY_PUBSUB:
        return "PUBSUB";
    case UA_LOGCATEGORY_DISCOVERY:
        return "DISCOVERY";
    }
    return "UNKNOWN";
}

[[nodiscard]] auto msg_args_to_string(const char *msg, va_list args) -> std::string {
    UA_String ua_string = UA_STRING_NULL;
    const auto clear_ua_string = std::experimental::scope_exit([&] { UA_String_clear(&ua_string); });

    const UA_StatusCode ua_status_code = UA_String_vformat(&ua_string, msg, args);
    if (ua_status_code != UA_STATUSCODE_GOOD || ua_string.data == nullptr || ua_string.length == 0) {
        return msg != nullptr ? std::string(msg) : std::string{};
    }

    const std::span<const UA_Byte> bytes{ua_string.data, ua_string.length};
    std::string out_str(ua_string.length, '\0');
    std::ranges::copy(bytes, out_str.begin());
    return out_str;
}

auto write_to_ilogger(logger::ILogger &ilogger, UA_LogLevel level, const std::string &line) -> void {
    switch (level) {
    case UA_LOGLEVEL_TRACE:
        ilogger.trace(line);
        break;
    case UA_LOGLEVEL_DEBUG:
        ilogger.debug(line);
        break;
    case UA_LOGLEVEL_INFO:
        ilogger.info(line);
        break;
    case UA_LOGLEVEL_WARNING:
        ilogger.warning(line);
        break;
    case UA_LOGLEVEL_ERROR:
        ilogger.error(line);
        break;
    case UA_LOGLEVEL_FATAL:
        ilogger.fatal(line);
        break;
    default:
        break;
    }
}

} // namespace internal

auto make_ua_logger_bridge(logger::ILogger &ilogger) -> UA_Logger {
    return UA_Logger{
        .log =
            [](void *log_context, UA_LogLevel level, UA_LogCategory category, const char *msg, va_list args) {
                if (log_context == nullptr || msg == nullptr) {
                    return;
                }
                std::stringstream stream;
                stream << "INTERNAL/" << internal::category_to_string(category) << ": "
                       << internal::msg_args_to_string(msg, args);
                auto &ilogger = *static_cast<logger::ILogger *>(log_context);
                internal::write_to_ilogger(ilogger, level, stream.str());
            },
        .context = static_cast<void *>(&ilogger),
        .clear =
            [](UA_Logger *ua_logger) {
                if (ua_logger != nullptr) {
                    ua_logger->context = nullptr;
                }
            },
    };
}

} // namespace opcua::detail