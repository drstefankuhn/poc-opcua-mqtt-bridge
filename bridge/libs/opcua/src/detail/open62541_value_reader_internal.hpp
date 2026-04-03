#pragma once

#include "opcua/path_value.hpp"

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/types.h>

#include <optional>
#include <string_view>

namespace opcua::detail::internal {

/**
 * @brief Indirection table for open62541 C APIs used by the value reader.
 *
 * @details
 * Production code uses open62541_value_reader_ops(); tests inject custom
 * function pointers (stubs) for unit-level isolation.
 */
struct Open62541ValueReaderOps {
    /**
     * @brief Read the browse name of a node.
     *
     * @param [in]  client   Connected open62541 client.
     * @param [in]  node_id  Node whose browse name is requested.
     * @param [out] out      Receives the qualified browse name.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*read_browse_name)(UA_Client *client, const UA_NodeId node_id, UA_QualifiedName *out);

    /**
     * @brief Iterate over the child nodes of a parent.
     *
     * @param [in] client          Connected open62541 client.
     * @param [in] parent_node_id  Parent node to enumerate.
     * @param [in] callback        Called once per child node.
     * @param [in] handle          Opaque context pointer passed to @p callback.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*for_each_child)(UA_Client *client, UA_NodeId parent_node_id, UA_NodeIteratorCallback callback,
                                    void *handle);

    /**
     * @brief Read the value attribute of a node.
     *
     * @param [in]  client   Connected open62541 client.
     * @param [in]  node_id  Node whose value is requested.
     * @param [out] out      Receives the value as a `UA_Variant`.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*read_value_attribute)(UA_Client *client, const UA_NodeId node_id, UA_Variant *out);

    /**
     * @brief Deep-copy a `UA_NodeId`.
     *
     * @param [in]  src  Source node ID.
     * @param [out] dst  Destination node ID.
     *
     * @return `UA_STATUSCODE_GOOD` on success.
     */
    UA_StatusCode (*node_id_copy)(const UA_NodeId *src, UA_NodeId *dst);
};

/**
 * @brief Return the production ops table pointing to real open62541 functions.
 *
 * @return An Open62541ValueReaderOps populated with the default open62541 API calls.
 */
[[nodiscard]] inline auto open62541_value_reader_ops() noexcept -> Open62541ValueReaderOps {
    return Open62541ValueReaderOps{
        .read_browse_name = UA_Client_readBrowseNameAttribute,
        .for_each_child = UA_Client_forEachChildNodeCall,
        .read_value_attribute = UA_Client_readValueAttribute,
        .node_id_copy = UA_NodeId_copy,
    };
}

/**
 * @brief Find a direct child node by browse name.
 *
 * @param [in]  ops            Ops table for open62541 calls.
 * @param [in]  client         Connected open62541 client.
 * @param [in]  parent         Parent node to search under.
 * @param [in]  child_name     Browse name to look for.
 * @param [out] child_node_id  Receives the found child's node ID on success.
 *
 * @retval true   Child was found; @p child_node_id is valid.
 * @retval false  Child was not found or an error occurred.
 */
[[nodiscard]] auto find_direct_child_by_name(const Open62541ValueReaderOps &ops, UA_Client *client,
                                             const UA_NodeId &parent, std::string_view child_name,
                                             UA_NodeId *child_node_id) -> bool;

/**
 * @brief Resolve a slash-separated path to a node ID starting from the Objects folder.
 *
 * @param [in]  ops            Ops table for open62541 calls.
 * @param [in]  client         Connected open62541 client.
 * @param [in]  path           Slash-separated node path (e.g. `"Demo/Counter"`).
 * @param [out] child_node_id  Receives the resolved node ID on success.
 *
 * @retval true   Path was resolved; @p child_node_id is valid.
 * @retval false  Path could not be resolved or an error occurred.
 */
[[nodiscard]] auto find_child_by_path(const Open62541ValueReaderOps &ops, UA_Client *client, std::string_view path,
                                      UA_NodeId *child_node_id) -> bool;

/**
 * @brief Convert a `UA_Variant` to a PathValue::Value.
 *
 * @details
 * Supports boolean, signed/unsigned integers, floats, doubles, strings,
 * and byte arrays. Returns `std::monostate` for unsupported or empty variants.
 *
 * @param [in] ua_variant  open62541 variant to convert.
 *
 * @return The converted value, or `std::monostate` if the type is unsupported.
 */
[[nodiscard]] auto ua_variant_to_std_variant(const UA_Variant &ua_variant) noexcept -> PathValue::Value;

/**
 * @brief Read a node value by path using the given ops table.
 *
 * @param [in] ops     Ops table for open62541 calls.
 * @param [in] client  Connected open62541 client.
 * @param [in] path    Slash-separated node path.
 *
 * @return The value if the node was found and readable, or `std::nullopt`.
 */
[[nodiscard]] auto read_value(const Open62541ValueReaderOps &ops, UA_Client *client, std::string_view path)
    -> std::optional<PathValue::Value>;

} // namespace opcua::detail::internal
