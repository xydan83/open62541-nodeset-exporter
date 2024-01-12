//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_LOGGER_STDLOG_H
#define NODESETEXPORTER_LOGGER_STDLOG_H

#include "nodesetexporter/common/LoggerBase.h"

#include <chrono>
#include <iomanip>
#include <iostream>

namespace nodesetexporter::logger
{

// ANSI Formatting Codes
const std::string reset = "\033[m";
const std::string bold = "\033[1m";
const std::string dark = "\033[2m";
const std::string underline = "\033[4m";
const std::string blink = "\033[5m";
const std::string reverse = "\033[7m";
const std::string concealed = "\033[8m";
const std::string clear_line = "\033[K";

// ANSI Color Codes
const std::string black = "\033[30m";
const std::string red = "\033[31m";
const std::string green = "\033[32m";
const std::string yellow = "\033[33m";
const std::string blue = "\033[34m";
const std::string magenta = "\033[35m";
const std::string cyan = "\033[36m";
const std::string white = "\033[37m";

using std::chrono::system_clock;

/**
 * @brief The simplest implementation of outputting logging messages to stdout with support for color (ANSI) message status designations.
 * @tparam TString an object that supports strings with operator<< implemented.
 */
class ConsoleLogger final : public common::LoggerBase<std::string>
{
public:
    explicit ConsoleLogger(std::string&& logger_name)
        : LoggerBase<std::string>(std::move(logger_name)){};

private:
    [[nodiscard]] inline auto GetMeCurrentDT() const
    {
        auto m_time_chrono = system_clock::to_time_t(system_clock::now());
        return std::put_time(std::localtime(&m_time_chrono), "%F %T");
    }

    void VDebug(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] "
                  << " [debug] " << message << std::endl;
    }
    void VInfo(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] " << green << " [info] " << reset << message << std::endl;
    }
    void VWarning(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] " << yellow << " [warning] " << reset << message << std::endl;
    }
    void VError(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] " << red << " [error] " << reset << message << std::endl;
    }
    void VCritical(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] " << red << bold << " [critical] " << reset << message << std::endl;
    }
    void VTrace(std::string&& message) override
    {
        std::cout << "[" << GetMeCurrentDT() << "]"
                  << " [" << this->GetLoggerName() << "] "
                  << " [trace] " << message << std::endl;
    }
};


} // namespace nodesetexporter::logger

#endif // NODESETEXPORTER_LOGGER_STDLOG_H
