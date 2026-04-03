#include "opcua/open62541_opcua.hpp"

#include "detail/open62541_logger.hpp"
#include "detail/open62541_opcua_internal.hpp"
#include "detail/open62541_value_reader.hpp"
#include "detail/path_value_internal.hpp"

#include <open62541/client_config_default.h>

namespace opcua {

/**
 * @brief Private implementation data for Open62541OpcUa (pimpl idiom).
 */
struct Open62541OpcUa::Open62541OpcUaImpl {
    Open62541OpcUaImpl(detail::internal::Open62541OpcUaOps ops, logger::ILogger &logger, const std::string &host,
                       uint16_t port, const std::string &path)
        : logger(&logger), endpoint(std::string("opc.tcp://") + host + ":" + std::to_string(port) + path),
          client(UA_Client_new(), UA_Client_delete), _ua_logger(detail::make_ua_logger_bridge(logger)), ops(ops) {
        UA_ClientConfig_setDefault(UA_Client_getConfig(client.get()));

        // Route open62541 logs through our ILogger
        UA_ClientConfig *config = UA_Client_getConfig(client.get());
        config->logging = &_ua_logger;
        if (config->eventLoop != nullptr) {
            config->eventLoop->logger = &_ua_logger;
        }
    }
    logger::ILogger *logger;
    std::string endpoint;
    std::unique_ptr<UA_Client, void (*)(UA_Client *)> client;
    UA_Logger _ua_logger;
    detail::internal::Open62541OpcUaOps ops;
};

auto detail::internal::Open62541OpcUaFactory::make_with_ops(detail::internal::Open62541OpcUaOps ops,
                                                            logger::ILogger &logger, const std::string &host,
                                                            uint16_t port, const std::string &path)
    -> std::expected<Open62541OpcUa, std::string> {
    try {
        logger.info("Creating OPC UA client");
        Open62541OpcUa open62541_opcua;
        open62541_opcua._impl = std::make_unique<Open62541OpcUa::Open62541OpcUaImpl>(ops, logger, host, port, path);
        logger.info("Created.");
        return {std::move(open62541_opcua)};
    } catch (const std::exception &e) {
        return std::unexpected("Failed to create OPC UA client: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to create OPC UA client");
    }
}

auto Open62541OpcUa::make(logger::ILogger &logger, const std::string &host, uint16_t port, const std::string &path)
    -> std::expected<Open62541OpcUa, std::string> {
    return detail::internal::Open62541OpcUaFactory::make_with_ops(
        detail::internal::Open62541OpcUaOps{
            .connect = UA_Client_connect,
            .disconnect = UA_Client_disconnect,
            .status_code_name = UA_StatusCode_name,
            .read_value = detail::read_value,
            .set_path_value = detail::internal::PathValueMutator::set_path_value,
        },
        logger, host, port, path);
}

Open62541OpcUa::~Open62541OpcUa() noexcept {
    if (!_impl) {
        return;
    }
    try {
        _impl->logger->info("Destroyed client.");
    } catch (...) {
        static_cast<void>(0); // noexcept destructor: ignore all
    }
}

Open62541OpcUa::Open62541OpcUa(Open62541OpcUa &&other) noexcept = default;

auto Open62541OpcUa::operator=(Open62541OpcUa &&other) noexcept -> Open62541OpcUa & = default;

auto Open62541OpcUa::connect() noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("OPC UA client not created");
    }
    try {
        _impl->logger->info("Connecting to: " + _impl->endpoint);
        const UA_StatusCode ua_status_code = _impl->ops.connect(_impl->client.get(), _impl->endpoint.c_str());
        if (ua_status_code == UA_STATUSCODE_GOOD) {
            _impl->logger->info("Connected.");
            return {};
        }
        return std::unexpected("Failed to connect: " + std::string(_impl->ops.status_code_name(ua_status_code)));
    } catch (const std::exception &e) {
        return std::unexpected("Failed to connect: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to connect");
    }
}

auto Open62541OpcUa::read(std::span<PathValue> pathValues) noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("OPC UA client not created");
    }
    try {
        _impl->logger->info("Reading pathValues from: " + _impl->endpoint);
        for (auto &pathValue : pathValues) {
            if (auto opt_value_variant = _impl->ops.read_value(_impl->client.get(), pathValue.path())) {
                _impl->ops.set_path_value(pathValue, opt_value_variant.value());
            }
        }
        _impl->logger->info("Read.");
        return {};

    } catch (const std::exception &e) {
        return std::unexpected("Failed to read: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to read");
    }
}

auto Open62541OpcUa::disconnect() noexcept -> std::expected<void, std::string> {
    if (!_impl) {
        return std::unexpected("OPC UA client not created");
    }
    try {
        _impl->logger->info("Disconnecting from: " + _impl->endpoint);
        const UA_StatusCode ua_status_code = _impl->ops.disconnect(_impl->client.get());
        if (ua_status_code == UA_STATUSCODE_GOOD) {
            _impl->logger->info("Disconnected.");
            return {};
        }
        return std::unexpected("Failed to disconnect: " + std::string(_impl->ops.status_code_name(ua_status_code)));
    } catch (const std::exception &e) {
        return std::unexpected("Failed to disconnect: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Failed to disconnect");
    }
}

} // namespace opcua
