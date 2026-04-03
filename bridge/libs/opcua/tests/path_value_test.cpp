#include "detail/path_value_internal.hpp"
#include "opcua/path_value.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using opcua::PathValue;
using opcua::detail::internal::PathValueMutator;

TEST_CASE("PathValue constructor initializes path and default value", "[opcua][path_value]") {
    SECTION("provided path is stored and default value is monostate") {
        const PathValue path_value("Demo/Counter");
        REQUIRE(path_value.path() == "Demo/Counter");
        REQUIRE(std::holds_alternative<std::monostate>(path_value.value()));
    }

    SECTION("default constructor stores empty path and monostate") {
        const PathValue path_value;
        REQUIRE(path_value.path().empty());
        REQUIRE(std::holds_alternative<std::monostate>(path_value.value()));
    }
}

TEST_CASE("PathValue mutator updates variant alternatives", "[opcua][path_value]") {
    SECTION("string and byte vector updates") {
        PathValue path_value("Demo/MachineState");
        PathValueMutator::set_path_value(path_value, std::string("RUNNING"));
        REQUIRE(std::holds_alternative<std::string>(path_value.value()));
        REQUIRE(std::get<std::string>(path_value.value()) == "RUNNING");

        const std::vector<std::uint8_t> bytes{1U, 2U, 3U};
        PathValueMutator::set_path_value(path_value, bytes);
        REQUIRE(std::holds_alternative<std::vector<std::uint8_t>>(path_value.value()));
        REQUIRE(std::get<std::vector<std::uint8_t>>(path_value.value()) == bytes);
    }

    SECTION("scalar alternatives bool int32 and double") {
        PathValue path_value("Demo/Flag");
        PathValueMutator::set_path_value(path_value, true);
        REQUIRE(std::holds_alternative<bool>(path_value.value()));
        REQUIRE(std::get<bool>(path_value.value()));

        constexpr std::int32_t expected_int = -77;
        PathValueMutator::set_path_value(path_value, expected_int);
        REQUIRE(std::holds_alternative<std::int32_t>(path_value.value()));
        REQUIRE(std::get<std::int32_t>(path_value.value()) == expected_int);

        constexpr double expected_double = 12.5;
        PathValueMutator::set_path_value(path_value, expected_double);
        REQUIRE(std::holds_alternative<double>(path_value.value()));
        REQUIRE(std::get<double>(path_value.value()) == expected_double);
    }

    SECTION("value can be reset to monostate") {
        PathValue path_value("Demo/Resettable");
        PathValueMutator::set_path_value(path_value, std::string("ACTIVE"));
        REQUIRE(std::holds_alternative<std::string>(path_value.value()));

        PathValueMutator::set_path_value(path_value, std::monostate{});
        REQUIRE(std::holds_alternative<std::monostate>(path_value.value()));
    }
}

TEST_CASE("PathValue move operations preserve path and value", "[opcua][path_value]") {
    SECTION("move constructor keeps moved content") {
        constexpr double expected_value = 23.5;
        PathValue original("Demo/Temperature");
        PathValueMutator::set_path_value(original, expected_value);

        PathValue moved(std::move(original));
        REQUIRE(moved.path() == "Demo/Temperature");
        REQUIRE(std::holds_alternative<double>(moved.value()));
        REQUIRE(std::get<double>(moved.value()) == expected_value);
    }

    SECTION("move assignment transfers path and value") {
        PathValue source("Demo/Pressure");
        constexpr std::int32_t expected_value = 1234;
        PathValueMutator::set_path_value(source, expected_value);

        PathValue target("Other/Path");
        PathValueMutator::set_path_value(target, std::string("old"));
        target = std::move(source);

        REQUIRE(target.path() == "Demo/Pressure");
        REQUIRE(std::holds_alternative<std::int32_t>(target.value()));
        REQUIRE(std::get<std::int32_t>(target.value()) == expected_value);
    }
}

} // namespace
