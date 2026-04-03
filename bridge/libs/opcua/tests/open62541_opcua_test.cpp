#include "detail/open62541_opcua_internal.hpp"
#include "detail/path_value_internal.hpp"
#include "opcua/open62541_opcua.hpp"

#include "logger/ilogger.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct SilentLogger final : logger::ILogger {
    auto trace([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
    auto debug([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
    auto warning([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
    auto error([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
    auto fatal([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
    auto info([[maybe_unused]] const std::string &message, [[maybe_unused]] const std::source_location &location)
        -> void override {}
};

auto test_state_connect_status_code() -> UA_StatusCode & {
    static thread_local UA_StatusCode code{UA_STATUSCODE_GOOD};
    return code;
}

auto test_state_disconnect_status_code() -> UA_StatusCode & {
    static thread_local UA_StatusCode code{UA_STATUSCODE_GOOD};
    return code;
}

auto test_state_throw_on_read() -> bool & {
    static thread_local bool value{false};
    return value;
}

auto test_state_read_value_calls() -> int & {
    static thread_local int value{0};
    return value;
}

auto test_state_set_path_value_calls() -> int & {
    static thread_local int value{0};
    return value;
}

auto stub_connect([[maybe_unused]] UA_Client *client, [[maybe_unused]] const char *endpoint_url) -> UA_StatusCode {
    return test_state_connect_status_code();
}

auto stub_disconnect([[maybe_unused]] UA_Client *client) -> UA_StatusCode {
    return test_state_disconnect_status_code();
}

auto stub_status_code_name([[maybe_unused]] UA_StatusCode code) -> const char * { return "BADSTATUS"; }

auto stub_read_value([[maybe_unused]] UA_Client *client, std::string_view path)
    -> std::optional<opcua::PathValue::Value> {
    constexpr std::int32_t expected_counter_value = 7;
    ++test_state_read_value_calls();
    if (test_state_throw_on_read()) {
        throw std::runtime_error("read failure");
    }
    if (path == "Demo/Counter") {
        return expected_counter_value;
    }
    return std::nullopt;
}

auto stub_set_path_value(opcua::PathValue &path_value, const opcua::PathValue::Value &value) -> void {
    ++test_state_set_path_value_calls();
    opcua::detail::internal::PathValueMutator::set_path_value(path_value, value);
}

const opcua::detail::internal::Open62541OpcUaOps k_stub_open62541_ops{
    .connect = stub_connect,
    .disconnect = stub_disconnect,
    .status_code_name = stub_status_code_name,
    .read_value = stub_read_value,
    .set_path_value = stub_set_path_value,
};
constexpr std::uint16_t k_test_port = 4840;

TEST_CASE("open62541 opcua connect and disconnect follow injected ops", "[opcua][open62541][open62541_opcua]") {
    SilentLogger logger;
    test_state_connect_status_code() = UA_STATUSCODE_GOOD;
    test_state_disconnect_status_code() = UA_STATUSCODE_GOOD;
    auto created = opcua::detail::internal::Open62541OpcUaFactory::make_with_ops(k_stub_open62541_ops, logger,
                                                                                 "localhost", k_test_port, "/");
    REQUIRE(created.has_value());
    auto client = std::move(created.value());

    SECTION("connect and disconnect succeed") {
        REQUIRE(client.connect().has_value());
        REQUIRE(client.disconnect().has_value());
    }

    SECTION("connect failure reports status code name") {
        test_state_connect_status_code() = UA_STATUSCODE_BADCONNECTIONREJECTED;
        const auto result = client.connect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to connect: BADSTATUS");
    }

    SECTION("disconnect failure reports status code name") {
        test_state_disconnect_status_code() = UA_STATUSCODE_BADCONNECTIONCLOSED;
        const auto result = client.disconnect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to disconnect: BADSTATUS");
    }
}

TEST_CASE("open62541 opcua read uses injected value reader and mutator", "[opcua][open62541][open62541_opcua]") {
    SilentLogger logger;
    test_state_throw_on_read() = false;
    test_state_read_value_calls() = 0;
    test_state_set_path_value_calls() = 0;
    auto created = opcua::detail::internal::Open62541OpcUaFactory::make_with_ops(k_stub_open62541_ops, logger,
                                                                                 "localhost", k_test_port, "/");
    REQUIRE(created.has_value());
    auto client = std::move(created.value());

    std::vector<opcua::PathValue> path_values;
    path_values.emplace_back("Demo/Counter");
    path_values.emplace_back("Demo/Unknown");

    const auto result = client.read(std::span<opcua::PathValue>(path_values));
    REQUIRE(result.has_value());
    REQUIRE(test_state_read_value_calls() == 2);
    REQUIRE(test_state_set_path_value_calls() == 1);
    REQUIRE(std::holds_alternative<std::int32_t>(path_values[0].value()));
    REQUIRE(std::get<std::int32_t>(path_values[0].value()) == 7);
    REQUIRE(std::holds_alternative<std::monostate>(path_values[1].value()));
}

TEST_CASE("open62541 opcua read handles thrown exceptions", "[opcua][open62541][open62541_opcua]") {
    SilentLogger logger;
    test_state_throw_on_read() = true;
    test_state_read_value_calls() = 0;
    test_state_set_path_value_calls() = 0;
    auto created = opcua::detail::internal::Open62541OpcUaFactory::make_with_ops(k_stub_open62541_ops, logger,
                                                                                 "localhost", k_test_port, "/");
    REQUIRE(created.has_value());
    auto client = std::move(created.value());
    std::vector<opcua::PathValue> path_values;
    path_values.emplace_back("Demo/Counter");

    const auto result = client.read(std::span<opcua::PathValue>(path_values));
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == "Failed to read: read failure");
}

TEST_CASE("open62541 opcua moved-from object rejects operations", "[opcua][open62541][open62541_opcua]") {
    SilentLogger logger;
    auto created = opcua::detail::internal::Open62541OpcUaFactory::make_with_ops(k_stub_open62541_ops, logger,
                                                                                 "localhost", k_test_port, "/");
    REQUIRE(created.has_value());
    auto source = std::move(created.value());
    auto moved_to = std::move(source);

    std::vector<opcua::PathValue> path_values;
    path_values.emplace_back("Demo/Counter");

    // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
    REQUIRE_FALSE(source.connect().has_value());
    REQUIRE_FALSE(source.disconnect().has_value());
    REQUIRE_FALSE(source.read(std::span<opcua::PathValue>(path_values)).has_value());

    REQUIRE(moved_to.connect().has_value());
}

} // namespace
