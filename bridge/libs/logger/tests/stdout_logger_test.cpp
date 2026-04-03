#include "logger/stdout_logger.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <iostream>
#include <source_location>
#include <sstream>
#include <string>

namespace {

struct CoutCapture {
    std::ostringstream captured;
    std::streambuf *original{};

    CoutCapture() : original(std::cout.rdbuf(captured.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(original); }

    CoutCapture(const CoutCapture &) = delete;
    auto operator=(const CoutCapture &) -> CoutCapture & = delete;
    CoutCapture(CoutCapture &&) = delete;
    auto operator=(CoutCapture &&) -> CoutCapture & = delete;

    [[nodiscard]] auto str() const -> std::string { return captured.str(); }
};

TEST_CASE("stdout logger formats all log levels correctly", "[logger][stdout_logger]") {
    logger::StdoutLogger log("TestModule");
    const auto location = std::source_location::current();

    SECTION("trace outputs TRACE level tag") {
        CoutCapture capture;
        log.trace("trace message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TRACE"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("trace message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }

    SECTION("debug outputs DEBUG level tag") {
        CoutCapture capture;
        log.debug("debug message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("DEBUG"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("debug message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }

    SECTION("info outputs INFO level tag") {
        CoutCapture capture;
        log.info("info message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("INFO"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("info message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }

    SECTION("warning outputs WARNING level tag") {
        CoutCapture capture;
        log.warning("warning message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("WARNING"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("warning message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }

    SECTION("error outputs ERROR level tag") {
        CoutCapture capture;
        log.error("error message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("ERROR"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("error message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }

    SECTION("fatal outputs FATAL level tag") {
        CoutCapture capture;
        log.fatal("fatal message", location);
        const auto output = capture.str();
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("FATAL"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("TestModule"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("fatal message"));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
        REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
    }
}

TEST_CASE("stdout logger includes source location in output", "[logger][stdout_logger]") {
    logger::StdoutLogger log("LocTest");
    CoutCapture capture;
    const auto location = std::source_location::current();
    log.info("location check", location);
    const auto output = capture.str();

    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(location.file_name()));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring(std::to_string(location.line())));
}

TEST_CASE("stdout logger output format contains timestamp-like pattern", "[logger][stdout_logger]") {
    logger::StdoutLogger log("FmtTest");
    CoutCapture capture;
    log.info("fmt check");
    const auto output = capture.str();

    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("["));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("]"));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("FmtTest"));
    REQUIRE_THAT(output, Catch::Matchers::EndsWith("\n"));
}

TEST_CASE("stdout logger with empty module name omits module label gracefully", "[logger][stdout_logger]") {
    logger::StdoutLogger log;
    CoutCapture capture;
    log.info("no module");
    const auto output = capture.str();

    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("INFO"));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("no module"));
}

TEST_CASE("stdout logger handles empty message", "[logger][stdout_logger]") {
    logger::StdoutLogger log("EmptyMsg");
    CoutCapture capture;
    log.info("");
    const auto output = capture.str();

    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("INFO"));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("EmptyMsg"));
    REQUIRE_FALSE(output.empty());
}

TEST_CASE("stdout logger multiple calls produce separate lines", "[logger][stdout_logger]") {
    logger::StdoutLogger log("MultiTest");
    CoutCapture capture;
    log.info("first");
    log.info("second");
    const auto output = capture.str();

    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("first"));
    REQUIRE_THAT(output, Catch::Matchers::ContainsSubstring("second"));

    const auto first_newline = output.find('\n');
    REQUIRE(first_newline != std::string::npos);
    const auto second_newline = output.find('\n', first_newline + 1);
    REQUIRE(second_newline != std::string::npos);
}

} // namespace
