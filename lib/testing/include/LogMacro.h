//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2025 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef TESTING_LOGMACRO_H
#define TESTING_LOGMACRO_H

#include "nodesetexporter/common/DateTime.h"
#include "nodesetexporter/common/LoggerBase.h"

#include <doctest/doctest.h>

using LogerBase = nodesetexporter::common::LoggerBase<std::string>;
using DateTime = nodesetexporter::common::DateTime;

#define PREPARE_MESSAGE(message) (GetLoggerName() + "[" + DateTime::GetDateTimeToString(*DateTime::DateTimeLocalNow(), "%Y/%m/%d %H:%M:%S") + "]: " + (std::move(message))) // NOLINT

// Определяю класс в виде макроса, чтобы явно показывать в модуле назначение присутствие логирования.
#define TEST_LOGGER_INIT                                                                                                                                                                               \
    class Logger final : public LogerBase                                                                                                                                                              \
    {                                                                                                                                                                                                  \
    public:                                                                                                                                                                                            \
        explicit Logger(std::string&& logger_name)                                                                                                                                                     \
            : LoggerBase<std::string>(std::move(logger_name))                                                                                                                                          \
        {                                                                                                                                                                                              \
        }                                                                                                                                                                                              \
                                                                                                                                                                                                       \
    private:                                                                                                                                                                                           \
        void VTrace(std::string&& message) override                                                                                                                                                    \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
        void VDebug(std::string&& message) override                                                                                                                                                    \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
        void VInfo(std::string&& message) override                                                                                                                                                     \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
        void VWarning(std::string&& message) override                                                                                                                                                  \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
        void VError(std::string&& message) override                                                                                                                                                    \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
        void VCritical(std::string&& message) override                                                                                                                                                 \
        {                                                                                                                                                                                              \
            MESSAGE(PREPARE_MESSAGE(message));                                                                                                                                                         \
        }                                                                                                                                                                                              \
    };

#endif // TESTING_LOGMACRO_H
