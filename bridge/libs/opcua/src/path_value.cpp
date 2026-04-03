#include "opcua/path_value.hpp"

#include "detail/path_value_internal.hpp"

#include <string_view>

namespace opcua {

PathValue::PathValue(std::string_view path) : _path(path) {}

PathValue::~PathValue() noexcept = default;

PathValue::PathValue(PathValue &&other) noexcept = default;
auto PathValue::operator=(PathValue &&other) noexcept -> PathValue & = default;

auto PathValue::path() const noexcept -> std::string_view { return _path; }

auto PathValue::value() const noexcept -> const PathValue::Value & { return _value; }

namespace detail::internal {

auto PathValueMutator::set_path_value(PathValue &path_value, const PathValue::Value &value) -> void {
    path_value._value = value;
}

} // namespace detail::internal

} // namespace opcua