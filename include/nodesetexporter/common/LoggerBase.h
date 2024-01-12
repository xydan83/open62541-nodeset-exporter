//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

//***** EXAMPLE OF INHERITANCE WITH LOGING LIBRARY spdlog ******

// #include "spdlog/sinks/stdout_color_sinks.h"
// #include "spdlog/spdlog.h"
//
//
//  template <typename TString>
//  class Logger final : public LoggerBase<TString>
//{
//  public:
//     explicit Logger(TString&& logger_name)
//         : LoggerBase<TString>(std::move(logger_name))
//         , m_console(spdlog::stdout_color_mt("console"))
//     {
//         m_console->set_level(spdlog::level::trace);
//     };
//
//  private:
//     std::shared_ptr<spdlog::logger> m_console;
//
//     void VDebug(TString&& message) override
//     {
//         m_console->debug(std::move(message));
//     }
//     void VInfo(TString&& message) override
//     {
//         m_console->info(std::move(message));
//     }
//     void VWarning(TString&& message) override
//     {
//         m_console->warn(std::move(message));
//     }
//     void VError(TString&& message) override
//     {
//         m_console->error(std::move(message));
//     }
//     void VCritical(TString&& message) override
//     {
//         m_console->critical(std::move(message));
//     }
//     void VTrace(TString&& message) override
//     {
//         m_console->trace(std::move(message));
//     }
// };
//***** EXAMPLE OF USE WITH LOGING LIBRARY spdlog ******
//     common::Logger<std::string> logger("nodesetexporter");
//     logger.SetLevel(common::LogLevel::Off);
//     logger.Trace("TRACE");
//     logger.Debug("DEBUG");
//     logger.Info("INFO");
//     logger.Error("ERROR");
//     logger.Critical("CRITICAL");
//**************************************************************


#ifndef NODESETEXPORTER_COMMON_LOGGERBASE_H
#define NODESETEXPORTER_COMMON_LOGGERBASE_H

#include <fmt/format.h>

#include <stdexcept>

namespace nodesetexporter::common
{

enum class LogLevel : int
{
    All = 0,
    Trace = 1,
    Debug = 2,
    Info = 3,
    Warning = 4,
    Error = 5,
    Critical = 6,
    Off = 7
};

/**
 * @brief Base class for performing logging with different implementations.
 *        Partially uses the Non-Virtual Interface idiom.
 *        https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Non-Virtual_Interface
 *        The operation of output modes (file, console, etc.) must be described in a derived class.
 * @tparam TString class representing tools for working with strings.
 */
template <typename TString>
class LoggerBase
{
public:
    /**
     * @brief Outputting messages to the log indicating the logging level
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Log(LogLevel log_level, fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        switch (log_level)
        {
        case LogLevel::All:
            throw std::runtime_error("Specify a specific logging level for the message.");
        case LogLevel::Trace:
            Trace(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Debug:
            Debug(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Info:
            Info(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Warning:
            Warning(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Error:
            Error(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Critical:
            Critical(fmt, std::forward<TArgs>(args)...);
            break;
        case LogLevel::Off:
            break;
        default:
            throw std::runtime_error("The specified logging level is not supported.");
        }
    };

    /**
     * @brief Outputting trace messages
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Trace(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Trace))
        {
            return;
        }
        VTrace(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Debug Message Output
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Debug(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Debug))
        {
            return;
        }
        VDebug(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Output of information messages
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Info(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Info))
        {
            return;
        }
        VInfo(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Displaying warning messages
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Warning(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Warning))
        {
            return;
        }
        VWarning(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Displaying error messages
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Error(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Error))
        {
            return;
        }
        VError(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Output of critical messages
     * @tparam TArgs
     * @param fmt
     * @param args
     */
    template <typename... TArgs>
    void Critical(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        if (!IsEnable(LogLevel::Critical))
        {
            return;
        }
        VCritical(fmt::format(fmt, std::forward<TArgs>(args)...));
    };

    /**
     * @brief Setting the logging level
     * @param log_level Logging level
     */
    virtual void SetLevel(LogLevel log_level)
    {
        m_log_level = log_level;
    };

    /**
     * @brief Getting the logging level
     * @return Logging level
     */
    [[nodiscard]] virtual LogLevel GetLevel() const
    {
        return m_log_level;
    };

    /**
     * @brief Getting the logger name
     * @return Logger name of string template type
     */
    [[nodiscard]] virtual TString const& GetLoggerName() const
    {
        return m_logger_name;
    };

    /**
     * @brief Setting a pattern to output data in a certain sequence and with a certain set
     * @param pattern String template type pattern
     */
    virtual void SetPattern(TString&& /*pattern*/)
    {
        throw std::runtime_error("not resolve");
    };

    /**
     * @brief Obtaining a set pattern for outputting data in a certain sequence and with a certain set
     * @return String template type pattern
     */
    [[nodiscard]] virtual TString const& GetPattern() const
    {
        throw std::runtime_error("not resolve");
    };

    /**
     * @brief The method is used to determine the logging level.
     * @param log_level Verified logging level for activity
     * @return true - if the level being checked is active
     */
    [[nodiscard]] constexpr bool IsEnable(const LogLevel log_level) const noexcept
    {
        switch (log_level)
        {
        case LogLevel::Off:
            return m_log_level == LogLevel::Off;
        default:
            return log_level >= m_log_level;
        }
    }

    explicit LoggerBase(TString&& logger_name)
        : m_logger_name(std::move(logger_name)){};
    virtual ~LoggerBase() = default;
    LoggerBase(LoggerBase const&) noexcept = delete;
    LoggerBase& operator=(LoggerBase const&) = delete;
    LoggerBase(LoggerBase&&) noexcept = delete;
    LoggerBase& operator=(LoggerBase&&) = delete;

private:
    LogLevel m_log_level{LogLevel::All}; // by default we accept all
    const TString m_logger_name;

    virtual void VTrace(TString&& message) = 0;
    virtual void VDebug(TString&& message) = 0;
    virtual void VInfo(TString&& message) = 0;
    virtual void VWarning(TString&& message) = 0;
    virtual void VError(TString&& message) = 0;
    virtual void VCritical(TString&& message) = 0;
};


} // namespace nodesetexporter::common
#endif // NODESETEXPORTER_COMMON_LOGGERBASE_H