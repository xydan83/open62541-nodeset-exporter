//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright 2024 (c) Aleksander Rozhkov <aleksprog@hotmail.com>
//

#include "nodesetexporter/logger/LogPlugin.h"

#include <array>

namespace nodesetexporter::logger
{

UA_Logger Open62541LogPlugin::Open62541LoggerCreator(LoggerBase& logger)
{
    UA_Logger ua_log;
    ua_log.log = UaLogStdoutLog;
    ua_log.context = &logger;
    ua_log.clear = nullptr;

    return ua_log;
}

inline void Open62541LogPlugin::UaLogStdoutLog(void* context, UA_LogLevel level, UA_LogCategory category, const char* msg, va_list args)
{
    auto* logger = static_cast<LoggerBase*>(context);

    switch (level)
    {
    case UA_LOGLEVEL_TRACE:
        if (logger->IsEnable(LogLevel::Trace))
        {
            ToLog(logger, LogLevel::Trace, category, msg, args);
        }
        break;
    case UA_LOGLEVEL_DEBUG:
        if (logger->IsEnable(LogLevel::Debug))
        {
            ToLog(logger, LogLevel::Debug, category, msg, args);
        }
        break;
    case UA_LOGLEVEL_INFO:
        if (logger->IsEnable(LogLevel::Info))
        {
            ToLog(logger, LogLevel::Info, category, msg, args);
        }
        break;
    case UA_LOGLEVEL_WARNING:
        if (logger->IsEnable(LogLevel::Warning))
        {
            ToLog(logger, LogLevel::Warning, category, msg, args);
        }
        break;
    case UA_LOGLEVEL_ERROR:
        if (logger->IsEnable(LogLevel::Error))
        {
            ToLog(logger, LogLevel::Error, category, msg, args);
        }
        break;
    case UA_LOGLEVEL_FATAL:
        if (logger->IsEnable(LogLevel::Critical))
        {
            ToLog(logger, LogLevel::Critical, category, msg, args);
        }
        break;
    }
}

inline std::string LogCategoryEnumToString(UA_LogCategory log) noexcept
{
    switch (log)
    {
    case UA_LOGCATEGORY_NETWORK:
        return "UA_LOGCATEGORY_NETWORK";
    case UA_LOGCATEGORY_SECURECHANNEL:
        return "UA_LOGCATEGORY_SECURECHANNEL";
    case UA_LOGCATEGORY_SESSION:
        return "UA_LOGCATEGORY_SESSION";
    case UA_LOGCATEGORY_SERVER:
        return "UA_LOGCATEGORY_SERVER";
    case UA_LOGCATEGORY_CLIENT:
        return "UA_LOGCATEGORY_CLIENT";
    case UA_LOGCATEGORY_USERLAND:
        return "UA_LOGCATEGORY_USERLAND";
    case UA_LOGCATEGORY_SECURITYPOLICY:
        return "UA_LOGCATEGORY_SECURITYPOLICY";
    }
    return "UA_UNKNOWN";
}

inline void Open62541LogPlugin::ToLog(LoggerBase* logger, LogLevel level, UA_LogCategory& category, const char* msg, va_list args)
{
    // Since the buffer is static, memory will be allocated once and the buffer will be used without relocation.
    // There is no need to overwrite the buffer either, since the end of the line will be determined by the line terminator.
    static std::array<char, txt_buffer_size> formatted;
    auto num = vsnprintf(formatted.data(), formatted.size(), msg, args);
    logger->Log(level, "[{}] {}", LogCategoryEnumToString(category), std::string(formatted.data(), num));
}
} // namespace nodesetexporter::logger
