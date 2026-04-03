#pragma once

#include "logger/ilogger.hpp"

#include <string>

namespace logger {

/**
 * @brief ILogger implementation that writes formatted log lines to `std::cout`.
 *
 * @details
 * Each line is formatted as
 * `[<timestamp> <module_name>] <LEVEL> <file>:<line> <message>`.
 * The timestamp uses second-precision UTC.
 */
class StdoutLogger : public ILogger {
  public:
    /**
     * @brief Construct a StdoutLogger.
     *
     * @param [in] module_name  Label prepended to every log line (may be empty).
     */
    explicit StdoutLogger(std::string module_name = "");

    /**
     * @brief Log a message at TRACE level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto trace(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

    /**
     * @brief Log a message at DEBUG level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto debug(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

    /**
     * @brief Log a message at INFO level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto info(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

    /**
     * @brief Log a message at WARNING level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto warning(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

    /**
     * @brief Log a message at ERROR level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto error(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

    /**
     * @brief Log a message at FATAL level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    auto fatal(const std::string &message, const std::source_location &location = std::source_location::current())
        -> void override;

  private:
    std::string _module_name;
};

} // namespace logger
