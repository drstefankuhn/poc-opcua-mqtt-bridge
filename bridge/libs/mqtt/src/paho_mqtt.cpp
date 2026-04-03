#include "mqtt/paho_mqtt.hpp"

#include "detail/paho_mqtt_internal.hpp"

#include <mqtt/async_client.h>

#include <memory>
#include <utility>

namespace mqtt {

namespace {

/** @brief Default MQTT client identifier used for all connections. */
constexpr auto client_id = std::string("mqtt-client");

/**
 * @brief Production connect op: connect via `mqtt::async_client`.
 *
 * @param [in] client  Opaque pointer to the underlying `mqtt::async_client`.
 * @param [in] broker  Broker URI (unused; the client already knows the broker).
 */
void paho_connect(void *client, [[maybe_unused]] const std::string &broker) {
    auto *async_client = static_cast<::mqtt::async_client *>(client);
    ::mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);
    async_client->connect(conn_opts)->wait();
}

/**
 * @brief Production publish op: publish via `mqtt::async_client`.
 *
 * @param [in] client   Opaque pointer to the underlying `mqtt::async_client`.
 * @param [in] topic    MQTT topic.
 * @param [in] message  Payload to publish.
 */
void paho_publish(void *client, const std::string &topic, const std::string &message) {
    auto *async_client = static_cast<::mqtt::async_client *>(client);
    async_client->publish(topic, message.data(), message.size(), 1, false)->wait();
}

/**
 * @brief Production disconnect op: disconnect via `mqtt::async_client`.
 *
 * @param [in] client  Opaque pointer to the underlying `mqtt::async_client`.
 */
void paho_disconnect(void *client) {
    auto *async_client = static_cast<::mqtt::async_client *>(client);
    async_client->disconnect()->wait();
}

} // namespace

/**
 * @brief Private implementation data for PahoMqtt (pimpl idiom).
 */
struct PahoMqtt::PahoMqttImpl {
    PahoMqttImpl(detail::internal::PahoMqttOps ops, logger::ILogger &logger, const std::string &host, uint16_t port,
                 std::string topic)
        : logger(&logger), broker(std::string("tcp://") + host + ":" + std::to_string(port)), topic(std::move(topic)),
          client(std::make_unique<::mqtt::async_client>(broker, client_id)), ops(ops) {}
    logger::ILogger *logger;
    std::string broker;
    std::string topic;
    std::unique_ptr<::mqtt::async_client> client;
    detail::internal::PahoMqttOps ops;
};

auto detail::internal::PahoMqttFactory::make_with_ops(PahoMqttOps ops, logger::ILogger &logger, const std::string &host,
                                                      uint16_t port, std::string topic)
    -> std::expected<PahoMqtt, std::string> {
    try {
        logger.info("Creating MQTT client with ID: " + client_id);
        PahoMqtt paho_mqtt;
        paho_mqtt._impl = std::make_unique<PahoMqtt::PahoMqttImpl>(ops, logger, host, port, std::move(topic));
        logger.info("Created.");
        return {std::move(paho_mqtt)};
    } catch (const std::exception &e) {
        return std::unexpected("Failed to create MQTT client: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to create MQTT client");
    }
}

auto PahoMqtt::make(logger::ILogger &logger, const std::string &host, uint16_t port, std::string topic)
    -> std::expected<PahoMqtt, std::string> {
    return detail::internal::PahoMqttFactory::make_with_ops(
        detail::internal::PahoMqttOps{
            .connect = paho_connect,
            .publish = paho_publish,
            .disconnect = paho_disconnect,
        },
        logger, host, port, std::move(topic));
}

PahoMqtt::~PahoMqtt() noexcept {
    if (!_impl) {
        return;
    }
    try {
        _impl->logger->info("Destroyed client with ID: " + client_id);
    } catch (...) {
        static_cast<void>(0); // noexcept destructor: ignore all
    }
}

PahoMqtt::PahoMqtt(PahoMqtt &&other) noexcept = default;

auto PahoMqtt::operator=(PahoMqtt &&other) noexcept -> PahoMqtt & = default;

auto PahoMqtt::connect() noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("MQTT client not created");
    }
    try {
        _impl->logger->info("Connecting to: " + _impl->broker);
        _impl->ops.connect(_impl->client.get(), _impl->broker);
        _impl->logger->info("Connected.");
        return {};
    } catch (const std::exception &e) {
        return std::unexpected("Failed to connect: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to connect");
    }
}

auto PahoMqtt::publish(const std::string &message) noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("MQTT client not created");
    }
    try {
        _impl->logger->info("Publishing message '" + message + "' to topic: '" + _impl->topic + "'");
        _impl->ops.publish(_impl->client.get(), _impl->topic, message);
        _impl->logger->info("Published.");
        return {};
    } catch (const std::exception &e) {
        return std::unexpected("Failed to publish: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to publish");
    }
}

auto PahoMqtt::disconnect() noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("MQTT client not created");
    }
    try {
        _impl->logger->info("Disconnecting from: " + _impl->broker);
        _impl->ops.disconnect(_impl->client.get());
        _impl->logger->info("Disconnected.");
        return {};
    } catch (const std::exception &e) {
        return std::unexpected("Failed to disconnect: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to disconnect");
    }
}

} // namespace mqtt
