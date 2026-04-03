#include "detail/open62541_value_reader.hpp"
#include "detail/open62541_value_reader_internal.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <experimental/scope>
#include <memory>
#include <string>
#include <vector>

namespace {

using opcua::detail::internal::find_child_by_path;
using opcua::detail::internal::find_direct_child_by_name;
using opcua::detail::internal::open62541_value_reader_ops;
using opcua::detail::internal::Open62541ValueReaderOps;
using opcua::detail::internal::read_value;
using opcua::detail::internal::ua_variant_to_std_variant;

// -----------------------------------------------------------------------------
// Shared test helpers (stubs, op tables)
// -----------------------------------------------------------------------------

[[nodiscard]] auto stub_ua_client() -> UA_Client * {
    // Non-null token for stub ops; will never be dereferenced.
    alignas(std::max_align_t) static std::byte storage{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<UA_Client *>(std::addressof(storage));
}

auto stub_read_browse_name_succeeds_demo(UA_Client * /*client*/, const UA_NodeId /*node_id*/, UA_QualifiedName *out)
    -> UA_StatusCode {
    UA_QualifiedName_init(out);
    out->namespaceIndex = 0;
    out->name = UA_STRING_ALLOC("Demo");
    return UA_STATUSCODE_GOOD;
}

auto stub_read_browse_name_fails(UA_Client * /*client*/, const UA_NodeId /*node_id*/, UA_QualifiedName * /*out*/)
    -> UA_StatusCode {
    return UA_STATUSCODE_BADNODEIDUNKNOWN;
}

auto test_state_call_count_stub_read_browse_name_succeeds_two_segments_in_order() -> int & {
    static thread_local int test_state_call_count{0};
    return test_state_call_count;
}

auto stub_read_browse_name_succeeds_two_segments_in_order(UA_Client * /*client*/, const UA_NodeId /*node_id*/,
                                                          UA_QualifiedName *out) -> UA_StatusCode {
    UA_QualifiedName_init(out);
    out->namespaceIndex = 0;
    const int browse_index =
        std::min(test_state_call_count_stub_read_browse_name_succeeds_two_segments_in_order()++, 1);
    out->name = UA_STRING_ALLOC((browse_index == 0) ? "Seg1" : "Seg2");
    return UA_STATUSCODE_GOOD;
}

auto stub_for_each_child_succeeds_single_child(UA_Client * /*client*/, UA_NodeId /*parent*/,
                                               UA_NodeIteratorCallback callback, void *handle) -> UA_StatusCode {
    const UA_NodeId child = UA_NODEID_NUMERIC(1, 99);
    return callback(child, UA_FALSE, UA_NODEID_NULL, handle);
}

auto stub_for_each_child_fails(UA_Client * /*client*/, UA_NodeId /*parent*/, UA_NodeIteratorCallback /*callback*/,
                               void * /*handle*/) -> UA_StatusCode {
    return UA_STATUSCODE_BADUNEXPECTEDERROR;
}

auto stub_for_each_child_fails_inverse_ignored_by_callback(UA_Client * /*client*/, UA_NodeId /*parent*/,
                                                           UA_NodeIteratorCallback callback, void *handle)
    -> UA_StatusCode {
    return callback(UA_NODEID_NUMERIC(1, 1), UA_TRUE, UA_NODEID_NULL, handle);
}

auto test_state_call_count_stub_for_each_child_succeeds_first_child_then_second_child() -> int & {
    static thread_local int test_state_call_count{0};
    return test_state_call_count;
}

auto stub_for_each_child_succeeds_first_child_then_second_child(UA_Client * /*client*/, UA_NodeId /*parent*/,
                                                                UA_NodeIteratorCallback callback, void *handle)
    -> UA_StatusCode {
    const int segment_index = test_state_call_count_stub_for_each_child_succeeds_first_child_then_second_child()++;
    if (segment_index == 0) {
        constexpr UA_UInt32 k_stub_node_id_two_segment_path_first = 10;
        return callback(UA_NODEID_NUMERIC(1, k_stub_node_id_two_segment_path_first), UA_FALSE, UA_NODEID_NULL, handle);
    }
    if (segment_index == 1) {
        constexpr UA_UInt32 k_stub_node_id_two_segment_path_second = 20;
        return callback(UA_NODEID_NUMERIC(1, k_stub_node_id_two_segment_path_second), UA_FALSE, UA_NODEID_NULL, handle);
    }
    return UA_STATUSCODE_GOOD;
}

auto test_state_call_count_stub_for_each_child_succeeds_first_then_fails() -> int & {
    static thread_local int test_state_call_count{0};
    return test_state_call_count;
}

auto stub_for_each_child_succeeds_first_then_fails(UA_Client * /*client*/, UA_NodeId /*parent*/,
                                                   UA_NodeIteratorCallback callback, void *handle) -> UA_StatusCode {
    if (test_state_call_count_stub_for_each_child_succeeds_first_then_fails() == 0) {
        constexpr UA_UInt32 k_stub_node_id_fail_after_first_segment_first_child = 100;
        test_state_call_count_stub_for_each_child_succeeds_first_then_fails()++;
        return callback(UA_NODEID_NUMERIC(1, k_stub_node_id_fail_after_first_segment_first_child), UA_FALSE,
                        UA_NODEID_NULL, handle);
    }
    return UA_STATUSCODE_BADUNEXPECTEDERROR;
}

auto stub_read_value_succeeds_seven(UA_Client * /*client*/, const UA_NodeId /*node_id*/, UA_Variant *out)
    -> UA_StatusCode {
    UA_Variant_init(out);
    constexpr UA_Int32 value = 7;
    return UA_Variant_setScalarCopy(out, &value, &UA_TYPES[UA_TYPES_INT32]);
}

auto stub_read_value_fails(UA_Client * /*client*/, const UA_NodeId /*node_id*/, UA_Variant * /*out*/) -> UA_StatusCode {
    return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
}

auto stub_node_id_copy_fails(const UA_NodeId * /*src*/, UA_NodeId * /*dst*/) -> UA_StatusCode {
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

const Open62541ValueReaderOps k_stub_ops_demo_path{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_succeeds_single_child,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_two_segment_path{
    .read_browse_name = stub_read_browse_name_succeeds_two_segments_in_order,
    .for_each_child = stub_for_each_child_succeeds_first_child_then_second_child,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_fail_after_first_segment{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_succeeds_first_then_fails,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_for_each_fails{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_fails,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_inverse_reference_ignored{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_fails_inverse_ignored_by_callback,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_browse_fails{
    .read_browse_name = stub_read_browse_name_fails,
    .for_each_child = stub_for_each_child_succeeds_single_child,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = UA_NodeId_copy,
};

const Open62541ValueReaderOps k_stub_ops_node_id_copy_fails{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_succeeds_single_child,
    .read_value_attribute = stub_read_value_succeeds_seven,
    .node_id_copy = stub_node_id_copy_fails,
};

const Open62541ValueReaderOps k_stub_ops_read_value_attribute_fails{
    .read_browse_name = stub_read_browse_name_succeeds_demo,
    .for_each_child = stub_for_each_child_succeeds_single_child,
    .read_value_attribute = stub_read_value_fails,
    .node_id_copy = UA_NodeId_copy,
};

// =============================================================================
// opcua::detail::read_value (public API; production ops)
// =============================================================================

TEST_CASE("read_value rejects null client or empty path", "[opcua][open62541][value_reader]") {
    REQUIRE_FALSE(opcua::detail::read_value(nullptr, "Demo/Counter").has_value());
    REQUIRE_FALSE(opcua::detail::read_value(stub_ua_client(), "").has_value());
    // Note: Positive tests would require a real ua client here; instead we test with injected ops below.
}

// =============================================================================
// internal::find_child_by_path
// =============================================================================

TEST_CASE("internal find_child_by_path rejects null client or empty path or null output",
          "[opcua][open62541][value_reader][internal]") {
    const auto ops = open62541_value_reader_ops();

    UA_NodeId child_node_id{};
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });
    UA_NodeId_init(&child_node_id);
    REQUIRE_FALSE(find_child_by_path(ops, nullptr, "Demo/Counter", &child_node_id));
    REQUIRE_FALSE(find_child_by_path(ops, stub_ua_client(), "", &child_node_id));
    REQUIRE_FALSE(find_child_by_path(ops, stub_ua_client(), "Demo/Counter", nullptr));
}

TEST_CASE("internal find_child_by_path single segment with injected ops",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId expected = UA_NODEID_NUMERIC(1, 99);

    UA_NodeId child_node_id{};
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });
    UA_NodeId_init(&child_node_id);
    REQUIRE(find_child_by_path(k_stub_ops_demo_path, stub_ua_client(), "Demo", &child_node_id));
    REQUIRE(UA_NodeId_equal(&child_node_id, &expected));
}

TEST_CASE("internal find_child_by_path two segments and skipped empty segments",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId expected = UA_NODEID_NUMERIC(1, 20);
    UA_NodeId child_node_id{};
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });

    // Reset call counts
    test_state_call_count_stub_read_browse_name_succeeds_two_segments_in_order() = 0;
    test_state_call_count_stub_for_each_child_succeeds_first_child_then_second_child() = 0;

    UA_NodeId_init(&child_node_id);
    REQUIRE(find_child_by_path(k_stub_ops_two_segment_path, stub_ua_client(), "Seg1/Seg2", &child_node_id));
    REQUIRE(UA_NodeId_equal(&child_node_id, &expected));
    UA_NodeId_clear(&child_node_id);

    // Reset call counts
    test_state_call_count_stub_read_browse_name_succeeds_two_segments_in_order() = 0;
    test_state_call_count_stub_for_each_child_succeeds_first_child_then_second_child() = 0;

    UA_NodeId_init(&child_node_id);
    REQUIRE(find_child_by_path(k_stub_ops_two_segment_path, stub_ua_client(), "//Seg1//Seg2", &child_node_id));
    REQUIRE(UA_NodeId_equal(&child_node_id, &expected));
}

TEST_CASE("internal find_child_by_path fails when second segment cannot be resolved",
          "[opcua][open62541][value_reader][internal]") {
    // Reset call count
    test_state_call_count_stub_for_each_child_succeeds_first_then_fails() = 0;

    UA_NodeId child_node_id{};
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });
    UA_NodeId_init(&child_node_id);
    REQUIRE_FALSE(
        find_child_by_path(k_stub_ops_fail_after_first_segment, stub_ua_client(), "Demo/NonExistent", &child_node_id));
}

TEST_CASE("internal find_child_by_path browse name mismatch", "[opcua][open62541][value_reader][internal]") {
    UA_NodeId child_node_id{};
    const auto clear_child_node_id = std::experimental::scope_exit([&] { UA_NodeId_clear(&child_node_id); });
    UA_NodeId_init(&child_node_id);
    REQUIRE_FALSE(find_child_by_path(k_stub_ops_demo_path, stub_ua_client(), "Other", &child_node_id));
}

// =============================================================================
// internal::find_direct_child_by_name
// =============================================================================

TEST_CASE("internal find_direct_child_by_name input guards", "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    const auto ops = open62541_value_reader_ops();

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE_FALSE(find_direct_child_by_name(ops, nullptr, parent, "child", &out));
    REQUIRE_FALSE(find_direct_child_by_name(ops, stub_ua_client(), parent, "", &out));
    REQUIRE_FALSE(find_direct_child_by_name(ops, stub_ua_client(), parent, "child", nullptr));
}

TEST_CASE("internal find_direct_child_by_name fails when for_each_child returns error",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE_FALSE(find_direct_child_by_name(k_stub_ops_for_each_fails, stub_ua_client(), parent, "Demo", &out));
}

TEST_CASE("internal find_direct_child_by_name ignores inverse references in callback",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE_FALSE(
        find_direct_child_by_name(k_stub_ops_inverse_reference_ignored, stub_ua_client(), parent, "Demo", &out));
}

TEST_CASE("internal find_direct_child_by_name fails when read browse name fails",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE_FALSE(find_direct_child_by_name(k_stub_ops_browse_fails, stub_ua_client(), parent, "Demo", &out));
}

TEST_CASE("internal find_direct_child_by_name fails when node id copy fails",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE_FALSE(find_direct_child_by_name(k_stub_ops_node_id_copy_fails, stub_ua_client(), parent, "Demo", &out));
}

TEST_CASE("internal find_direct_child_by_name succeeds with injected 'valid' ops (demo node)",
          "[opcua][open62541][value_reader][internal]") {
    const UA_NodeId parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    const UA_NodeId expected = UA_NODEID_NUMERIC(1, 99);

    UA_NodeId out{};
    const auto clear_out = std::experimental::scope_exit([&] { UA_NodeId_clear(&out); });
    UA_NodeId_init(&out);
    REQUIRE(find_direct_child_by_name(k_stub_ops_demo_path, stub_ua_client(), parent, "Demo", &out));
    REQUIRE(UA_NodeId_equal(&out, &expected));
}

// =============================================================================
// internal::read_value (injected ops)
// =============================================================================

TEST_CASE("internal read_value input guards and failures", "[opcua][open62541][value_reader][internal]") {
    SECTION("null client") { REQUIRE_FALSE(read_value(k_stub_ops_demo_path, nullptr, "Demo").has_value()); }

    SECTION("empty path") { REQUIRE_FALSE(read_value(k_stub_ops_demo_path, stub_ua_client(), "").has_value()); }

    SECTION("path resolution fails") {
        REQUIRE_FALSE(read_value(k_stub_ops_demo_path, stub_ua_client(), "Other").has_value());
    }

    SECTION("read value attribute fails after path resolved") {
        REQUIRE_FALSE(read_value(k_stub_ops_read_value_attribute_fails, stub_ua_client(), "Demo").has_value());
    }
}

TEST_CASE("internal read_value success with injected 'valid' ops (demo node and value 7)",
          "[opcua][open62541][value_reader][internal]") {
    const auto value = read_value(k_stub_ops_demo_path, stub_ua_client(), "Demo");

    REQUIRE(value.has_value());
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access): guarded by REQUIRE(value.has_value())
    const opcua::PathValue::Value &unpacked = *value;
    REQUIRE(std::holds_alternative<std::int32_t>(unpacked));
    REQUIRE(std::get<std::int32_t>(unpacked) == 7);
}

// =============================================================================
// internal::ua_variant_to_std_variant
// =============================================================================

TEST_CASE("internal ua_variant_to_std_variant bool, uint/int types", "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("bool") {
        constexpr bool expected{true};
        constexpr UA_Boolean value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_BOOLEAN]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<bool>(out));
        REQUIRE(std::get<bool>(out) == expected);
    }

    SECTION("int8_t") {
        constexpr std::int8_t expected{-42};
        constexpr UA_SByte value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_SBYTE]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::int8_t>(out));
        REQUIRE(std::get<std::int8_t>(out) == expected);
    }

    SECTION("uint8_t") {
        constexpr std::uint8_t expected{31};
        constexpr UA_Byte value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_BYTE]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::uint8_t>(out));
        REQUIRE(std::get<std::uint8_t>(out) == expected);
    }

    SECTION("int16_t") {
        constexpr std::int16_t expected{-5800};
        constexpr UA_Int16 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_INT16]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::int16_t>(out));
        REQUIRE(std::get<std::int16_t>(out) == expected);
    }

    SECTION("uint16_t") {
        constexpr std::uint16_t expected{6789};
        constexpr UA_UInt16 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_UINT16]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::uint16_t>(out));
        REQUIRE(std::get<std::uint16_t>(out) == expected);
    }

    SECTION("int32_t") {
        constexpr std::int32_t expected{-1234567890};
        constexpr UA_Int32 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_INT32]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::int32_t>(out));
        REQUIRE(std::get<std::int32_t>(out) == expected);
    }

    SECTION("uint32_t") {
        constexpr std::uint32_t expected{135791234};
        constexpr UA_UInt32 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_UINT32]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::uint32_t>(out));
        REQUIRE(std::get<std::uint32_t>(out) == expected);
    }

    SECTION("int64_t") {
        constexpr std::int64_t expected{-9876543210};
        constexpr UA_Int64 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_INT64]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::int64_t>(out));
        REQUIRE(std::get<std::int64_t>(out) == expected);
    }

    SECTION("uint64_t") {
        constexpr std::uint64_t expected{9786543210};
        constexpr UA_UInt64 value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_UINT64]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::uint64_t>(out));
        REQUIRE(std::get<std::uint64_t>(out) == expected);
    }
}

TEST_CASE("internal ua_variant_to_std_variant float/double types", "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("float") {
        constexpr float expected{-13.21F};
        constexpr UA_Float value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_FLOAT]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<float>(out));
        REQUIRE(std::get<float>(out) == expected);
    }

    SECTION("double") {
        constexpr double expected{-4223.0313};
        constexpr UA_Double value{expected};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<double>(out));
        REQUIRE(std::get<double>(out) == expected);
    }
}

TEST_CASE("internal ua_variant_to_std_variant string type", "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("non-empty string") {
        constexpr std::string_view expected{"abc"};
        std::array<UA_Byte, 3> text_bytes{
            static_cast<UA_Byte>(expected[0]),
            static_cast<UA_Byte>(expected[1]),
            static_cast<UA_Byte>(expected[2]),
        };
        UA_String text{static_cast<size_t>(text_bytes.size()), text_bytes.data()};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &text, &UA_TYPES[UA_TYPES_STRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::string>(out));
        REQUIRE(std::get<std::string>(out) == expected);
    }

    SECTION("empty string") {
        UA_String empty{0, nullptr};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &empty, &UA_TYPES[UA_TYPES_STRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::string>(out));
        REQUIRE(std::get<std::string>(out).empty());
    }

    SECTION("string with embedded null byte") {
        std::array<UA_Byte, 3> text_bytes{
            static_cast<UA_Byte>('a'),
            static_cast<UA_Byte>(0),
            static_cast<UA_Byte>('b'),
        };
        UA_String text{static_cast<size_t>(text_bytes.size()), text_bytes.data()};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &text, &UA_TYPES[UA_TYPES_STRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::string>(out));
        const auto &string = std::get<std::string>(out);
        REQUIRE(string.size() == 3);
        REQUIRE(string[0] == 'a');
        REQUIRE(string[1] == '\0');
        REQUIRE(string[2] == 'b');
    }
}

TEST_CASE("internal ua_variant_to_std_variant bytestring type", "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("non-empty bytestring") {
        const std::vector<std::uint8_t> expected{9U, 8U};
        std::array<UA_Byte, 2> raw_bytes_pair{expected[0], expected[1]};
        UA_ByteString byte_string{static_cast<size_t>(raw_bytes_pair.size()), raw_bytes_pair.data()};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &byte_string, &UA_TYPES[UA_TYPES_BYTESTRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(out));
        REQUIRE(std::get<std::vector<std::uint8_t>>(out) == expected);
    }

    SECTION("empty bytestring") {
        UA_ByteString empty{0, nullptr};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &empty, &UA_TYPES[UA_TYPES_BYTESTRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(out));
        REQUIRE(std::get<std::vector<std::uint8_t>>(out).empty());
    }

    SECTION("bytestring keeps raw binary bytes") {
        const std::vector<std::uint8_t> expected{0x00U, 0x7FU, 0x80U, 0xFFU};
        std::array<UA_Byte, 4> raw{
            static_cast<UA_Byte>(expected[0]),
            static_cast<UA_Byte>(expected[1]),
            static_cast<UA_Byte>(expected[2]),
            static_cast<UA_Byte>(expected[3]),
        };
        UA_ByteString byte_string{static_cast<size_t>(raw.size()), raw.data()};
        REQUIRE(UA_Variant_setScalarCopy(&variant, &byte_string, &UA_TYPES[UA_TYPES_BYTESTRING]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(out));
        REQUIRE(std::get<std::vector<std::uint8_t>>(out) == expected);
    }
}

TEST_CASE("internal ua_variant_to_std_variant byte array type", "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("non-empty byte array") {
        const std::vector<std::uint8_t> expected_bytes{1U, 2U, 3U};
        std::array<UA_Byte, 3> raw_bytes{
            static_cast<UA_Byte>(expected_bytes[0]),
            static_cast<UA_Byte>(expected_bytes[1]),
            static_cast<UA_Byte>(expected_bytes[2]),
        };
        REQUIRE(UA_Variant_setArrayCopy(&variant, raw_bytes.data(), raw_bytes.size(), &UA_TYPES[UA_TYPES_BYTE]) ==
                UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(out));
        REQUIRE(std::get<std::vector<std::uint8_t>>(out) == expected_bytes);
    }

    SECTION("byte array with boundary byte values") {
        const std::vector<std::uint8_t> expected{0x00U, 0x7FU, 0x80U, 0xFFU};
        std::array<UA_Byte, 4> raw{
            static_cast<UA_Byte>(expected[0]),
            static_cast<UA_Byte>(expected[1]),
            static_cast<UA_Byte>(expected[2]),
            static_cast<UA_Byte>(expected[3]),
        };
        REQUIRE(UA_Variant_setArrayCopy(&variant, raw.data(), raw.size(), &UA_TYPES[UA_TYPES_BYTE]) ==
                UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(out));
        REQUIRE(std::get<std::vector<std::uint8_t>>(out) == expected);
    }

    SECTION("empty byte array maps to monostate") {
        // Implementation currently converts only Byte arrays with arrayLength > 0.
        std::array<UA_Byte, 1> dummy{0U};
        REQUIRE(UA_Variant_setArrayCopy(&variant, dummy.data(), 0, &UA_TYPES[UA_TYPES_BYTE]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }
}

TEST_CASE("internal ua_variant_to_std_variant returns monostate for unsupported/invalid inputs",
          "[opcua][open62541][value_reader][internal]") {
    UA_Variant variant{};
    const auto clear_variant = std::experimental::scope_exit([&] { UA_Variant_clear(&variant); });
    UA_Variant_init(&variant);

    SECTION("type and data are null") {
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }

    SECTION("type is set but data is null") {
        variant.type = &UA_TYPES[UA_TYPES_INT32];
        variant.data = nullptr;
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }

    SECTION("non-byte array is unsupported shape") {
        constexpr std::array<UA_Int32, 2> values{1, 2};
        REQUIRE(UA_Variant_setArrayCopy(&variant, values.data(), values.size(), &UA_TYPES[UA_TYPES_INT32]) ==
                UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }

    SECTION("array of otherwise supported scalar type is unsupported (Boolean[])") {
        constexpr std::array<UA_Boolean, 2> values{UA_TRUE, UA_FALSE};
        REQUIRE(UA_Variant_setArrayCopy(&variant, values.data(), values.size(), &UA_TYPES[UA_TYPES_BOOLEAN]) ==
                UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }

    SECTION("unsupported scalar type NodeId maps to monostate") {
        const UA_NodeId node_id = UA_NODEID_NUMERIC(1, 1234);
        REQUIRE(UA_Variant_setScalarCopy(&variant, &node_id, &UA_TYPES[UA_TYPES_NODEID]) == UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }

    SECTION("unsupported scalar type DateTime maps to monostate") {
        constexpr UA_DateTime date_time_scalar = 0;
        REQUIRE(UA_Variant_setScalarCopy(&variant, &date_time_scalar, &UA_TYPES[UA_TYPES_DATETIME]) ==
                UA_STATUSCODE_GOOD);
        const auto out = ua_variant_to_std_variant(variant);
        REQUIRE(std::holds_alternative<std::monostate>(out));
    }
}

} // namespace
