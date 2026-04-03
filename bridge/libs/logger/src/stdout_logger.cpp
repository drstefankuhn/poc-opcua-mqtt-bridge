#include "logger/stdout_logger.hpp"

#include <chrono>
#include <format>
#include <iostream>

namespace logger {

namespace {

/**
 * @brief Write a formatted log line to `std::cout`.
 *
 * @param [in] module_name  Module label (may be empty).
 * @param [in] level        Severity tag (e.g. `"INFO"`, `"ERROR"`).
 * @param [in] message      Text to log.
 * @param [in] location     Source location of the original call site.
 */
auto log(const std::string &module_name, const std::string &level, const std::string &message,
         const std::source_location &location) -> void {

    const std::chrono::sys_seconds sec{std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())};
    std::cout << std::format("[{:%Y-%m-%d %H:%M:%S} {}] {} {}:{} {}\n", sec, module_name, level, location.file_name(),
                             location.line(), message);
}

} // namespace

StdoutLogger::StdoutLogger(std::string module_name) : _module_name(std::move(module_name)) {}

auto StdoutLogger::trace(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "TRACE", message, location);
}

auto StdoutLogger::debug(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "DEBUG", message, location);
}

auto StdoutLogger::info(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "INFO", message, location);
}

auto StdoutLogger::warning(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "WARNING", message, location);
}

auto StdoutLogger::error(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "ERROR", message, location);
}

auto StdoutLogger::fatal(const std::string &message, const std::source_location &location) -> void {
    log(_module_name, "FATAL", message, location);
}

} // namespace logger