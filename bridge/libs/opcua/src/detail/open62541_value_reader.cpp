#include "open62541_value_reader.hpp"
#include "open62541_value_reader_internal.hpp"

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/types.h>

#include <algorithm>
#include <experimental/scope>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace opcua::detail {

namespace internal {

/**
 * @brief Context passed through the open62541 child-node iteration callback.
 */
struct FindChildContext {
    const Open62541ValueReaderOps *ops{};
    UA_Client *client{};
    std::string_view child_name;
    UA_NodeId found_node{};
    bool is_found{false};
};

namespace {

/**
 * @brief Callback invoked by open62541 for each child of a parent node.
 *
 * @details
 * Compares the child's browse name with the target name stored in the
 * FindChildContext. Sets `is_found` and copies the node ID on match.
 *
 * @param [in]     child_id             Node ID of the current child.
 * @param [in]     is_inverse           True if this is an inverse reference (skipped).
 * @param [in]     reference_type_id    Reference type (unused).
 * @param [in,out] handle               Pointer to a FindChildContext.
 *
 * @return Always `UA_STATUSCODE_GOOD` (iteration continues until exhausted).
 */
auto find_direct_child_by_name_cb(UA_NodeId child_id, UA_Boolean is_inverse,
                                  [[maybe_unused]] UA_NodeId reference_type_id, void *handle) -> UA_StatusCode {
    auto *context = static_cast<FindChildContext *>(handle);
    if (context == nullptr || context->ops == nullptr || context->client == nullptr || context->is_found ||
        is_inverse) {
        return UA_STATUSCODE_GOOD;
    }

    UA_QualifiedName browse_name;
    UA_QualifiedName_init(&browse_name);
    const auto clear_browse_name = std::experimental::scope_exit([&] { UA_QualifiedName_clear(&browse_name); });
    const UA_StatusCode ua_status_code = context->ops->read_browse_name(context->client, child_id, &browse_name);
    if (ua_status_code != UA_STATUSCODE_GOOD || browse_name.name.data == nullptr || browse_name.name.length == 0) {
        return UA_STATUSCODE_GOOD;
    }

    const std::span<const UA_Byte> bytes{browse_name.name.data, browse_name.name.length};
    std::string current_name(bytes.size(), '\0');
    std::ranges::copy(bytes, current_name.begin());
    if (current_name != context->child_name) {
        return UA_STATUSCODE_GOOD;
    }

    const UA_StatusCode ua_status_code2 = context->ops->node_id_copy(&child_id, &context->found_node);
    if (ua_status_code2 != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_GOOD;
    }

    context->is_found = true;
    return UA_STATUSCODE_GOOD;
}

} // namespace

auto find_direct_child_by_name(const Open62541ValueReaderOps &ops, UA_Client *client, const UA_NodeId &parent,
                               std::string_view child_name, UA_NodeId *child_node_id) -> bool {
    if (client == nullptr || child_name.empty() || child_node_id == nullptr) {
        return false;
    }

    FindChildContext context{
        .ops = &ops,
        .client = client,
        .child_name = child_name,
        .found_node = UA_NODEID_NULL,
        .is_found = false,
    };

    UA_NodeId_init(&context.found_node);
    const UA_StatusCode ua_status_code = ops.for_each_child(client, parent, find_direct_child_by_name_cb, &context);
    if (ua_status_code != UA_STATUSCODE_GOOD || !context.is_found) {
        UA_NodeId_clear(&context.found_node);
        return false;
    }

    *child_node_id = context.found_node;
    return true;
}

auto find_child_by_path(const Open62541ValueReaderOps &ops, UA_Client *client, std::string_view path,
                        UA_NodeId *child_node_id) -> bool {
    if (client == nullptr || path.empty() || child_node_id == nullptr) {
        return false;
    }

    UA_NodeId current_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bool current_owned = false;
    for (const auto &segment : path | std::views::split('/')) {
        std::string_view child_name{segment.begin(), segment.end()};
        if (child_name.empty()) {
            continue;
        }

        UA_NodeId next_node_id{};
        UA_NodeId_init(&next_node_id);
        if (!find_direct_child_by_name(ops, client, current_node_id, child_name, &next_node_id)) {
            if (current_owned) {
                UA_NodeId_clear(&current_node_id);
            }
            return false;
        }
        if (current_owned) {
            UA_NodeId_clear(&current_node_id);
        }
        current_node_id = next_node_id;
        current_owned = true;
    }
    *child_node_id = current_node_id;
    return true;
}

auto ua_variant_to_std_variant(const UA_Variant &ua_variant) noexcept -> PathValue::Value {
    if (ua_variant.type == nullptr || ua_variant.data == nullptr) {
        return std::monostate{};
    }
    // Byte array
    if (ua_variant.arrayLength > 0 && UA_Variant_hasArrayType(&ua_variant, &UA_TYPES[UA_TYPES_BYTE])) {
        const auto *ua_bytes = static_cast<const UA_Byte *>(ua_variant.data);
        std::vector<std::uint8_t> out(ua_variant.arrayLength);
        const std::span<const std::uint8_t> bytes{ua_bytes, ua_variant.arrayLength};
        std::ranges::copy(bytes, out.begin());
        return out;
    }
    // Scalars
    if (!UA_Variant_isScalar(&ua_variant)) {
        return std::monostate{};
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        return *static_cast<const UA_Boolean *>(ua_variant.data) == UA_TRUE;
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_SBYTE])) {
        return static_cast<std::int8_t>(*static_cast<const UA_SByte *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_BYTE])) {
        return static_cast<std::uint8_t>(*static_cast<const UA_Byte *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_INT16])) {
        return static_cast<std::int16_t>(*static_cast<const UA_Int16 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_UINT16])) {
        return static_cast<std::uint16_t>(*static_cast<const UA_UInt16 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_INT32])) {
        return static_cast<std::int32_t>(*static_cast<const UA_Int32 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_UINT32])) {
        return static_cast<std::uint32_t>(*static_cast<const UA_UInt32 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_INT64])) {
        return static_cast<std::int64_t>(*static_cast<const UA_Int64 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_UINT64])) {
        return static_cast<std::uint64_t>(*static_cast<const UA_UInt64 *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_FLOAT])) {
        return static_cast<float>(*static_cast<const UA_Float *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_DOUBLE])) {
        return static_cast<double>(*static_cast<const UA_Double *>(ua_variant.data));
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_STRING])) {
        const auto &ua_string = *static_cast<const UA_String *>(ua_variant.data);
        if (ua_string.data == nullptr || ua_string.length == 0) {
            return std::string{};
        }
        const std::span<const UA_Byte> bytes{ua_string.data, ua_string.length};
        std::string out(ua_string.length, '\0');
        std::ranges::copy(bytes, out.begin());
        return out;
    }
    if (UA_Variant_hasScalarType(&ua_variant, &UA_TYPES[UA_TYPES_BYTESTRING])) {
        const auto &ua_byte_string = *static_cast<const UA_ByteString *>(ua_variant.data);
        if (ua_byte_string.data == nullptr || ua_byte_string.length == 0) {
            return std::vector<std::uint8_t>{};
        }
        const std::span<const UA_Byte> bytes{ua_byte_string.data, ua_byte_string.length};
        std::vector<std::uint8_t> out(ua_byte_string.length);
        std::ranges::copy(bytes, out.begin());
        return out;
    }
    return std::monostate{};
}

auto read_value(const Open62541ValueReaderOps &ops, UA_Client *client, std::string_view path)
    -> std::optional<PathValue::Value> {
    if (client == nullptr) {
        return std::nullopt;
    }

    UA_NodeId child_node_id{};
    UA_NodeId_init(&child_node_id);
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });
    if (!find_child_by_path(ops, client, path, &child_node_id)) {
        return std::nullopt;
    }

    UA_Variant child_value{};
    UA_Variant_init(&child_value);
    const auto clear_child_value = std::experimental::scope_exit([&] { UA_Variant_clear(&child_value); });
    if (ops.read_value_attribute(client, child_node_id, &child_value) != UA_STATUSCODE_GOOD) {
        return std::nullopt;
    }

    return ua_variant_to_std_variant(child_value);
}

} // namespace internal

auto read_value(UA_Client *client, std::string_view path) -> std::optional<PathValue::Value> {
    return internal::read_value(internal::open62541_value_reader_ops(), client, path);
}

} // namespace opcua::detail
