#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace opcua {

namespace detail::internal {
struct PathValueMutator;
}

/**
 * @brief Holds an OPC UA node path together with its most recently read value.
 *
 * @details
 * Constructed with a node path; the value starts as `std::monostate` and is
 * populated by IOpcUa::read(). Move-only.
 */
class PathValue {
  public:
    /**
     * @brief Variant type representing possible OPC UA node values.
     *
     * @details
     * Holds `std::monostate` when no value has been read yet, or one of the
     * supported OPC UA scalar types (boolean, signed/unsigned integers, floats,
     * strings, byte arrays).
     */
    using Value =
        std::variant<std::monostate, bool, std::int8_t, std::uint8_t, std::int16_t, std::uint16_t, std::int32_t,
                     std::uint32_t, std::int64_t, std::uint64_t, float, double, std::string, std::vector<std::uint8_t>>;

    /**
     * @brief Construct a PathValue with the given node path.
     *
     * @param [in] path  OPC UA node path (e.g. `"Demo/Counter"`).
     */
    explicit PathValue(std::string_view path = "");

    /** @brief Destructor. */
    ~PathValue() noexcept;

    /** @brief Move constructor. */
    PathValue(PathValue &&other) noexcept;

    /** @brief Copying is not supported. */
    PathValue(const PathValue &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(PathValue &&other) noexcept -> PathValue &;

    /** @brief Copy assignment is not supported. */
    auto operator=(const PathValue &) -> PathValue & = delete;

    /**
     * @brief Return the node path.
     *
     * @return The OPC UA node path as a string view.
     */
    [[nodiscard]] auto path() const noexcept -> std::string_view;

    /**
     * @brief Return the current value.
     *
     * @return Reference to the value variant; `std::monostate` if not yet read.
     */
    [[nodiscard]] auto value() const noexcept -> const Value &;

  private:
    std::string _path;
    Value _value;

    friend struct opcua::detail::internal::PathValueMutator;
};

} // namespace opcua
