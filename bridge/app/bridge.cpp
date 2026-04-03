#include "bridge.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>

namespace bridge {

namespace {

/**
 * @brief Check an expected result; log to `std::cerr` on error.
 *
 * @tparam T  Value type.
 * @tparam E  Error type (must be streamable).
 * @param [in] result  Expected to check.
 *
 * @retval true   Result holds a value.
 * @retval false  Result holds an error (printed to `std::cerr`).
 */
template <typename T, typename E> [[nodiscard]] auto check(const std::expected<T, E> &result) -> bool {
    if (result) {
        return true;
    }
    std::cerr << "Error: " << result.error() << '\n';
    return false;
}

/**
 * @brief Serialize a span of PathValues to a JSON string.
 *
 * @details
 * Each PathValue becomes a key-value pair where the key is the node path and
 * the value is the read result (`null` for `std::monostate`).
 *
 * @param [in] pathValues  Path-value pairs to serialize.
 *
 * @return A JSON object string.
 */
[[nodiscard]] auto to_json(std::span<opcua::PathValue> pathValues) -> std::string {
    nlohmann::json json;

    for (const auto &path_value : pathValues) {
        const auto key = std::string(path_value.path());
        const auto value_variant = path_value.value();

        std::visit(
            [&](const auto &value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    json[key] = nullptr;
                } else {
                    json[key] = value;
                }
            },
            value_variant);
    }

    return json.dump();
}

} // namespace

Bridge::Bridge(opcua::IOpcUa &opcua, mqtt::IMqtt &mqtt, std::span<const std::string_view> opcua_paths)
    : _opcua(&opcua), _mqtt(&mqtt), _opcua_paths(opcua_paths.begin(), opcua_paths.end()) {}

auto Bridge::run() noexcept -> std::expected<void, std::string> {
    try {

        // Build PathValue list from configured OPC UA paths
        std::vector<opcua::PathValue> path_values;
        std::ranges::transform(_opcua_paths.cbegin(), _opcua_paths.cend(), std::back_inserter(path_values),
                               [](const auto &path) { return opcua::PathValue(path); });

        // Read values from OPC UA
        const bool read_success = check(_opcua->connect()) && check(_opcua->read({path_values}));
        if (!check(_opcua->disconnect()) || !read_success) {
            return std::unexpected("Failed to read from OPC UA");
        }

        // Publish JSON payload to MQTT
        const bool publish_success = check(_mqtt->connect()) && check(_mqtt->publish(to_json(path_values)));
        if (!check(_mqtt->disconnect()) || !publish_success) {
            return std::unexpected("Failed to publish to MQTT");
        }

        return {};

    } catch (const std::exception &e) {
        return std::unexpected("Unexpected exception: " + std::string(e.what()));
    } catch (...) {
        return std::unexpected("Unexpected exception");
    }
}

} // namespace bridge