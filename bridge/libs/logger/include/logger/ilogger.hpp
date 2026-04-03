#pragma once

#include <source_location>
#include <string>

namespace logger {

/**
 * @brief Abstract logging interface with six severity levels.
 *
 * @details
 * Defines the contract for all logger implementations. Each log method
 * accepts a message and an optional source location that defaults to the
 * caller's position. Implementations must be move-only.
 */
class ILogger {
  public:
    /** @brief Virtual destructor. */
    virtual ~ILogger() noexcept = default;

    /** @brief Move constructor. */
    ILogger(ILogger &&other) noexcept = default;

    /** @brief Copying is not supported. */
    ILogger(const ILogger &) = delete;

    /** @brief Move assignment operator. */
    auto operator=(ILogger &&other) noexcept -> ILogger & = default;

    /** @brief Copy assignment is not supported. */
    auto operator=(const ILogger &) -> ILogger & = delete;

    /**
     * @brief Log a message at TRACE level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto trace(const std::string &message,
                       const std::source_location &location = std::source_location::current()) -> void = 0;

    /**
     * @brief Log a message at DEBUG level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto debug(const std::string &message,
                       const std::source_location &location = std::source_location::current()) -> void = 0;

    /**
     * @brief Log a message at INFO level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto info(const std::string &message,
                      const std::source_location &location = std::source_location::current()) -> void = 0;

    /**
     * @brief Log a message at WARNING level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto warning(const std::string &message,
                         const std::source_location &location = std::source_location::current()) -> void = 0;

    /**
     * @brief Log a message at ERROR level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto error(const std::string &message,
                       const std::source_location &location = std::source_location::current()) -> void = 0;

    /**
     * @brief Log a message at FATAL level.
     *
     * @param [in] message   Text to log.
     * @param [in] location  Source location of the call site.
     */
    virtual auto fatal(const std::string &message,
                       const std::source_location &location = std::source_location::current()) -> void = 0;

  protected:
    /** @brief Default constructor (for derived classes only). */
    ILogger() = default;
};

} // namespace logger
