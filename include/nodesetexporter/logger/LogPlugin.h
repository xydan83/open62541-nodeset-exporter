//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#ifndef NODESETEXPORTER_LOGGER_LOGPLUGIN_H
#define NODESETEXPORTER_LOGGER_LOGPLUGIN_H

#include "nodesetexporter/common/LoggerBase.h"

#include <open62541/plugin/log.h>

#include <magic_enum.hpp>

namespace nodesetexporter::logger
{

using LoggerBase = nodesetexporter::common::LoggerBase<std::string>;
using LogLevel = nodesetexporter::common::LogLevel;

class Open62541LogPlugin final
{
public:
    static UA_Logger Open62541LoggerCreator(LoggerBase& logger);

private:
    static inline void UaLogStdoutLog(void* context, UA_LogLevel level, UA_LogCategory category, const char* msg, va_list args);

    static inline void ToLog(LoggerBase* logger, LogLevel level, UA_LogCategory& category, const char* msg, va_list args);

    constexpr static uint32_t txt_buffer_size = 1024;
};

} // namespace nodesetexporter::logger

#endif // NODESETEXPORTER_LOGGER_LOGPLUGIN_H