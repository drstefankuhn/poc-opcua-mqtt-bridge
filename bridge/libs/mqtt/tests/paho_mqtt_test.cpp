#include "detail/paho_mqtt_internal.hpp"
#include "mqtt/paho_mqtt.hpp"

#include "logger/ilogger.hpp"

#include <catch2/catch_test_macros.hpp>

#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct TestState {
    bool throw_on_connect{false};
    bool throw_nonstd_on_connect{false};
    bool throw_on_publish{false};
    bool throw_nonstd_on_publish{false};
    bool throw_on_disconnect{false};
    bool throw_nonstd_on_disconnect{false};
    int connect_calls{0};
    int publish_calls{0};
    int disconnect_calls{0};
    std::string last_publish_topic;
    std::string last_publish_message;
    std::string last_connect_broker;
    int throw_on_logger_info_call_index{0};
    int logger_info_calls{0};
};

auto test_state() -> TestState & {
    static thread_local TestState state{};
    return state;
}

auto reset_test_state() -> void { test_state() = TestState{}; }

void stub_connect([[maybe_unused]] void *client, [[maybe_unused]] const std::string &broker) {
    ++test_state().connect_calls;
    test_state().last_connect_broker = broker;
    if (test_state().throw_nonstd_on_connect) {
        // NOLINTNEXTLINE(hicpp-exception-baseclass)
        throw 1;
    }
    if (test_state().throw_on_connect) {
        throw std::runtime_error("connect failure");
    }
}

void stub_publish([[maybe_unused]] void *client, const std::string &topic, const std::string &message) {
    ++test_state().publish_calls;
    test_state().last_publish_topic = topic;
    test_state().last_publish_message = message;
    if (test_state().throw_nonstd_on_publish) {
        // NOLINTNEXTLINE(hicpp-exception-baseclass)
        throw 1;
    }
    if (test_state().throw_on_publish) {
        throw std::runtime_error("publish failure");
    }
}

void stub_disconnect([[maybe_unused]] void *client) {
    ++test_state().disconnect_calls;
    if (test_state().throw_nonstd_on_disconnect) {
        // NOLINTNEXTLINE(hicpp-exception-baseclass)
        throw 1;
    }
    if (test_state().throw_on_disconnect) {
        throw std::runtime_error("disconnect failure");
    }
}

const mqtt::detail::internal::PahoMqttOps k_stub_paho_ops{
    .connect = stub_connect,
    .publish = stub_publish,
    .disconnect = stub_disconnect,
};
constexpr std::uint16_t k_test_port = 1883;

struct TestLogger final : logger::ILogger {
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
        -> void override {
        ++test_state().logger_info_calls;
        if (test_state().throw_on_logger_info_call_index == test_state().logger_info_calls) {
            throw std::runtime_error("logger info failure");
        }
    }
};

auto make_stubbed_client(TestLogger &logger, std::string topic = "t") -> mqtt::PahoMqtt {
    auto created = mqtt::detail::internal::PahoMqttFactory::make_with_ops(k_stub_paho_ops, logger, "localhost",
                                                                           k_test_port, std::move(topic));
    REQUIRE(created.has_value());
    auto client = std::move(created.value());
    return client;
}

TEST_CASE("paho mqtt connect and disconnect follow injected ops", "[mqtt][paho_mqtt]") {
    TestLogger logger;
    reset_test_state();
    auto client = make_stubbed_client(logger);

    SECTION("connect and disconnect succeed") {
        REQUIRE(client.connect().has_value());
        REQUIRE(test_state().connect_calls == 1);
        REQUIRE(test_state().last_connect_broker == "tcp://localhost:1883");
        REQUIRE(client.disconnect().has_value());
        REQUIRE(test_state().disconnect_calls == 1);
    }

    SECTION("connect failure reports exception message") {
        test_state().throw_on_connect = true;
        const auto result = client.connect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to connect: connect failure");
    }

    SECTION("connect failure via non-standard exception hits generic error") {
        test_state().throw_nonstd_on_connect = true;
        const auto result = client.connect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to connect");
    }

    SECTION("disconnect failure reports exception message") {
        test_state().throw_on_disconnect = true;
        const auto result = client.disconnect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to disconnect: disconnect failure");
    }

    SECTION("disconnect failure via non-standard exception hits generic error") {
        test_state().throw_nonstd_on_disconnect = true;
        const auto result = client.disconnect();
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to disconnect");
    }
}

TEST_CASE("paho mqtt publish forwards topic and message through injected ops", "[mqtt][paho_mqtt]") {
    TestLogger logger;
    reset_test_state();
    auto client = make_stubbed_client(logger, "test/topic");

    SECTION("publish succeeds and passes correct topic and message") {
        const auto result = client.publish("hello");
        REQUIRE(result.has_value());
        REQUIRE(test_state().publish_calls == 1);
        REQUIRE(test_state().last_publish_topic == "test/topic");
        REQUIRE(test_state().last_publish_message == "hello");
    }

    SECTION("publish with empty message succeeds") {
        const auto result = client.publish("");
        REQUIRE(result.has_value());
        REQUIRE(test_state().publish_calls == 1);
        REQUIRE(test_state().last_publish_message.empty());
    }

    SECTION("publish failure reports exception message") {
        test_state().throw_on_publish = true;
        const auto result = client.publish("payload");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to publish: publish failure");
    }

    SECTION("publish failure via non-standard exception hits generic error") {
        test_state().throw_nonstd_on_publish = true;
        const auto result = client.publish("payload");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == "Failed to publish");
    }
}

TEST_CASE("paho mqtt full connect-publish-disconnect cycle", "[mqtt][paho_mqtt]") {
    TestLogger logger;
    reset_test_state();
    auto client = make_stubbed_client(logger, "sensor/data");

    REQUIRE(client.connect().has_value());
    REQUIRE(client.publish("msg1").has_value());
    REQUIRE(client.publish("msg2").has_value());
    REQUIRE(client.disconnect().has_value());

    REQUIRE(test_state().connect_calls == 1);
    REQUIRE(test_state().publish_calls == 2);
    REQUIRE(test_state().disconnect_calls == 1);
    REQUIRE(test_state().last_publish_topic == "sensor/data");
    REQUIRE(test_state().last_publish_message == "msg2");
}

TEST_CASE("paho mqtt moved-from object rejects operations", "[mqtt][paho_mqtt]") {
    TestLogger logger;
    reset_test_state();
    auto source = make_stubbed_client(logger);
    auto moved_to = std::move(source);

    // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move)
    const auto connect_result = source.connect();
    REQUIRE_FALSE(connect_result.has_value());
    REQUIRE(connect_result.error() == "MQTT client not created");

    const auto publish_result = source.publish("msg");
    REQUIRE_FALSE(publish_result.has_value());
    REQUIRE(publish_result.error() == "MQTT client not created");

    const auto disconnect_result = source.disconnect();
    REQUIRE_FALSE(disconnect_result.has_value());
    REQUIRE(disconnect_result.error() == "MQTT client not created");

    REQUIRE(moved_to.connect().has_value());
}

TEST_CASE("paho mqtt factory reports errors when logger info throws", "[mqtt][paho_mqtt]") {
    TestLogger logger;
    reset_test_state();
    test_state().throw_on_logger_info_call_index = 1;

    SECTION("throw during first info call returns exception message") {
        const auto created = mqtt::detail::internal::PahoMqttFactory::make_with_ops(k_stub_paho_ops, logger,
                                                                                    "localhost", k_test_port, "t");
        REQUIRE_FALSE(created.has_value());
        REQUIRE(created.error() == "Failed to create MQTT client: logger info failure");
    }

    SECTION("throw during second info call returns exception message") {
        reset_test_state();
        test_state().throw_on_logger_info_call_index = 2;
        const auto created = mqtt::detail::internal::PahoMqttFactory::make_with_ops(k_stub_paho_ops, logger,
                                                                                    "localhost", k_test_port, "t");
        REQUIRE_FALSE(created.has_value());
        REQUIRE(created.error() == "Failed to create MQTT client: logger info failure");
    }
}

} // namespace
