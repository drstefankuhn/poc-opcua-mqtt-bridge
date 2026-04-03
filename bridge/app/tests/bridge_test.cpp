#include "bridge.hpp"
#include "detail/path_value_internal.hpp"

#include "opcua/path_value.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <expected>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

namespace {

/// Silences std::cerr during error-path tests so that bridge::check()
/// output does not pollute test runner output.
struct CerrRedirect {
    std::ostringstream sink;
    std::streambuf *original;

    CerrRedirect() : original(std::cerr.rdbuf(sink.rdbuf())) {}
    ~CerrRedirect() { std::cerr.rdbuf(original); }

    CerrRedirect(const CerrRedirect &) = delete;
    auto operator=(const CerrRedirect &) -> CerrRedirect & = delete;
    CerrRedirect(CerrRedirect &&) = delete;
    auto operator=(CerrRedirect &&) -> CerrRedirect & = delete;
};

struct StubOpcUa final : opcua::IOpcUa {
    bool fail_connect{false};
    bool fail_read{false};
    bool fail_disconnect{false};
    int connect_calls{0};
    int read_calls{0};
    int disconnect_calls{0};
    opcua::PathValue::Value read_value_to_set;

    auto connect() noexcept -> std::expected<void, std::string> override {
        ++connect_calls;
        if (fail_connect) {
            return std::unexpected(std::string("opcua connect error"));
        }
        return {};
    }

    auto read(std::span<opcua::PathValue> path_values) noexcept -> std::expected<void, std::string> override {
        ++read_calls;
        if (fail_read) {
            return std::unexpected(std::string("opcua read error"));
        }
        if (!std::holds_alternative<std::monostate>(read_value_to_set)) {
            for (auto &path_value : path_values) {
                opcua::detail::internal::PathValueMutator::set_path_value(path_value, read_value_to_set);
            }
        }
        return {};
    }

    auto disconnect() noexcept -> std::expected<void, std::string> override {
        ++disconnect_calls;
        if (fail_disconnect) {
            return std::unexpected(std::string("opcua disconnect error"));
        }
        return {};
    }
};

struct StubMqtt final : mqtt::IMqtt {
    bool fail_connect{false};
    bool fail_publish{false};
    bool fail_disconnect{false};
    int connect_calls{0};
    int publish_calls{0};
    int disconnect_calls{0};
    std::string last_publish_message;

    auto connect() noexcept -> std::expected<void, std::string> override {
        ++connect_calls;
        if (fail_connect) {
            return std::unexpected(std::string("mqtt connect error"));
        }
        return {};
    }

    auto publish(const std::string &message) noexcept -> std::expected<void, std::string> override {
        ++publish_calls;
        last_publish_message = message;
        if (fail_publish) {
            return std::unexpected(std::string("mqtt publish error"));
        }
        return {};
    }

    auto disconnect() noexcept -> std::expected<void, std::string> override {
        ++disconnect_calls;
        if (fail_disconnect) {
            return std::unexpected(std::string("mqtt disconnect error"));
        }
        return {};
    }
};

TEST_CASE("bridge run performs full opcua-to-mqtt cycle", "[app][bridge]") {
    StubOpcUa opcua;
    StubMqtt mqtt;
    const auto paths = std::to_array<std::string_view>({"path/a", "path/b"});

    SECTION("succeeds and calls all operations") {
        constexpr std::int32_t expected_value{41};
        opcua.read_value_to_set = expected_value;
        bridge::Bridge bridge(opcua, mqtt, paths);
        REQUIRE(bridge.run().has_value());

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 1);
        REQUIRE(mqtt.publish_calls == 1);
        REQUIRE(mqtt.disconnect_calls == 1);
    }

    SECTION("publishes correct JSON with integer read values") {
        constexpr std::int32_t expected_value{43};
        opcua.read_value_to_set = expected_value;
        bridge::Bridge bridge(opcua, mqtt, paths);
        REQUIRE(bridge.run().has_value());

        const auto json = nlohmann::json::parse(mqtt.last_publish_message);
        REQUIRE(json["path/a"] == expected_value);
        REQUIRE(json["path/b"] == expected_value);
    }

    SECTION("publishes correct JSON with boolean read values") {
        constexpr bool expected_value = true;
        opcua.read_value_to_set = expected_value;
        bridge::Bridge bridge(opcua, mqtt, paths);
        REQUIRE(bridge.run().has_value());

        const auto json = nlohmann::json::parse(mqtt.last_publish_message);
        REQUIRE(json["path/a"] == expected_value);
        REQUIRE(json["path/b"] == expected_value);
    }

    SECTION("publishes null values for unset path values") {
        bridge::Bridge bridge(opcua, mqtt, paths);
        REQUIRE(bridge.run().has_value());

        const auto json = nlohmann::json::parse(mqtt.last_publish_message);
        REQUIRE(json["path/a"].is_null());
        REQUIRE(json["path/b"].is_null());
    }

    SECTION("publishes empty JSON object for empty paths") {
        bridge::Bridge bridge(opcua, mqtt, std::span<const std::string_view>{});
        REQUIRE(bridge.run().has_value());

        const auto json = nlohmann::json::parse(mqtt.last_publish_message);
        REQUIRE(json.empty());
    }
}

TEST_CASE("bridge run handles opcua errors", "[app][bridge]") {
    CerrRedirect cerr_redirect;
    StubOpcUa opcua;
    StubMqtt mqtt;
    const auto paths = std::to_array<std::string_view>({"path/a"});

    SECTION("connect failure returns error without calling read") {
        opcua.fail_connect = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to read from OPC UA");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 0);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 0);
    }

    SECTION("read failure returns error and still disconnects") {
        opcua.fail_read = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to read from OPC UA");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 0);
    }

    SECTION("disconnect failure returns error even when read succeeded") {
        opcua.fail_disconnect = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to read from OPC UA");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 0);
    }
}

TEST_CASE("bridge run handles mqtt errors", "[app][bridge]") {
    CerrRedirect cerr_redirect;
    StubOpcUa opcua;
    StubMqtt mqtt;
    const auto paths = std::to_array<std::string_view>({"path/a"});

    SECTION("connect failure returns error without calling publish") {
        mqtt.fail_connect = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to publish to MQTT");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 1);
        REQUIRE(mqtt.publish_calls == 0);
        REQUIRE(mqtt.disconnect_calls == 1);
    }

    SECTION("publish failure returns error and still disconnects") {
        mqtt.fail_publish = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to publish to MQTT");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 1);
        REQUIRE(mqtt.publish_calls == 1);
        REQUIRE(mqtt.disconnect_calls == 1);
    }

    SECTION("disconnect failure returns error even when publish succeeded") {
        mqtt.fail_disconnect = true;
        bridge::Bridge bridge(opcua, mqtt, paths);
        const auto result = bridge.run();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to publish to MQTT");

        REQUIRE(opcua.connect_calls == 1);
        REQUIRE(opcua.read_calls == 1);
        REQUIRE(opcua.disconnect_calls == 1);
        REQUIRE(mqtt.connect_calls == 1);
        REQUIRE(mqtt.publish_calls == 1);
        REQUIRE(mqtt.disconnect_calls == 1);
    }
}

} // namespace
