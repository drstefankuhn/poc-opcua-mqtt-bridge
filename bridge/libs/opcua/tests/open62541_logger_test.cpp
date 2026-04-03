#include "detail/open62541_logger.hpp"
#include "detail/open62541_logger_internal.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdarg>
#include <cstddef>
#include <source_location>
#include <string>

namespace {

struct CapturingLogger final : logger::ILogger {
    std::string last_level;
    std::string last_message;
    std::size_t calls{0U};

    auto trace(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "trace";
        last_message = message;
        ++calls;
    }

    auto debug(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "debug";
        last_message = message;
        ++calls;
    }

    auto info(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "info";
        last_message = message;
        ++calls;
    }

    auto warning(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "warning";
        last_message = message;
        ++calls;
    }

    auto error(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "error";
        last_message = message;
        ++calls;
    }

    auto fatal(const std::string &message, [[maybe_unused]] const std::source_location &location) -> void override {
        last_level = "fatal";
        last_message = message;
        ++calls;
    }
};

// NOLINTNEXTLINE(cert-dcl50-cpp)
auto msg_args_to_string_adapter(const char *msg, ...) -> std::string {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    va_list args;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
    va_start(args, msg);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
    const std::string out = opcua::detail::internal::msg_args_to_string(msg, args);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
    va_end(args);
    return out;
}

TEST_CASE("internal category_to_string maps known and unknown categories", "[opcua][open62541_logger][internal]") {
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_NETWORK) == "NETWORK");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_SECURECHANNEL) == "SECURECHANNEL");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_SESSION) == "SESSION");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_SERVER) == "SERVER");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_CLIENT) == "CLIENT");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_APPLICATION) == "APPLICATION");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_SECURITY) == "SECURITY");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_EVENTLOOP) == "EVENTLOOP");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_PUBSUB) == "PUBSUB");
    REQUIRE(opcua::detail::internal::category_to_string(UA_LOGCATEGORY_DISCOVERY) == "DISCOVERY");

    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    constexpr auto out_of_range_category = static_cast<UA_LogCategory>(UA_LOGCATEGORIES);
    REQUIRE(opcua::detail::internal::category_to_string(out_of_range_category) == "UNKNOWN");
}

TEST_CASE("internal msg_args_to_string formats and falls back", "[opcua][open62541_logger][internal]") {
    constexpr auto expected_number = 7;
    constexpr auto expected_number2 = 8;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    REQUIRE(msg_args_to_string_adapter("value=%d, value2=%i", expected_number, expected_number2) ==
            "value=7, value2=8");
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    REQUIRE(msg_args_to_string_adapter("plain") == "plain");
}

TEST_CASE("internal write_to_ilogger routes by level", "[opcua][open62541_logger][internal]") {
    CapturingLogger logger;
    const std::array cases{
        std::pair{UA_LOGLEVEL_TRACE, "trace"}, std::pair{UA_LOGLEVEL_DEBUG, "debug"},
        std::pair{UA_LOGLEVEL_INFO, "info"},   std::pair{UA_LOGLEVEL_WARNING, "warning"},
        std::pair{UA_LOGLEVEL_ERROR, "error"}, std::pair{UA_LOGLEVEL_FATAL, "fatal"},
    };

    for (const auto &[level, expected_level] : cases) {
        opcua::detail::internal::write_to_ilogger(logger, level, "line");
        REQUIRE(logger.last_level == expected_level);
        REQUIRE(logger.last_message == "line");
    }
}

TEST_CASE("logger bridge log callback forwards formatted line", "[opcua][open62541_logger]") {
    constexpr auto expected_number = 7;
    CapturingLogger logger;
    UA_Logger ua_logger = opcua::detail::make_ua_logger_bridge(logger);

    REQUIRE(ua_logger.context == static_cast<void *>(&logger));
    REQUIRE(ua_logger.log != nullptr);
    REQUIRE(ua_logger.clear != nullptr);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
    UA_LOG_INFO(&ua_logger, UA_LOGCATEGORY_CLIENT, "value=%d", expected_number);
    REQUIRE(logger.calls == 1U);
    REQUIRE(logger.last_level == "info");
    REQUIRE(logger.last_message.find("INTERNAL/CLIENT: value=7") != std::string::npos);
}

TEST_CASE("logger clear resets context pointer", "[opcua][open62541_logger]") {
    CapturingLogger logger;
    UA_Logger ua_logger = opcua::detail::make_ua_logger_bridge(logger);

    REQUIRE(ua_logger.context != nullptr);
    ua_logger.clear(&ua_logger);
    REQUIRE(ua_logger.context == nullptr);
}

} // namespace
